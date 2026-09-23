package com.amazon.kindle.bridge.network;

import com.amazon.kindle.bridge.NativeBridgeListener;
import com.amazon.kindle.bridge.NativeMessage;
import com.amazon.kindle.bridge.NativeProcessSupervisor;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Hashtable;

/**
 * Relays Java HTTP requests to a supervised native process.
 *
 * <p>The kindlet owns the request side of the relay: requests are encoded as
 * TYPE_HTTP_REQUEST frames and sent to the native daemon. Responses are
 * correlated by request ID when TYPE_HTTP_RESPONSE frames arrive. All other
 * native frames are forwarded to the application's listener.</p>
 *
 * Compatible with Java 1.4 / CDC 1.1.
 */
public final class NetworkRelayHandler implements NativeBridgeListener {

    private final NativeProcessSupervisor supervisor;
    private final NativeBridgeListener applicationListener;
    private final Hashtable pending = new Hashtable();
    private int nextRequestId = 1;

    /**
     * Constructs a relay around a native process supervisor.
     *
     * @param supervisor supervisor used to send frames to the daemon
     * @param applicationListener listener for non-HTTP native messages; may be null
     */
    public NetworkRelayHandler(NativeProcessSupervisor supervisor,
                               NativeBridgeListener applicationListener) {
        if (supervisor == null) {
            throw new IllegalArgumentException("supervisor must not be null");
        }
        this.supervisor = supervisor;
        this.applicationListener = applicationListener;
    }

    /** Constructs a relay without an application-level fallback listener. */
    public NetworkRelayHandler(NativeProcessSupervisor supervisor) {
        this(supervisor, null);
    }

    /**
     * Encodes and sends an HTTP request to the native daemon.
     *
     * @param request request to relay
     * @param listener callback for the correlated response or transport error
     * @return allocated monotonic request ID
     * @throws IOException if the frame cannot be written
     */
    public int sendHttpRequest(HttpRequest request, HttpResponseListener listener)
            throws IOException {
        if (request == null) {
            throw new IllegalArgumentException("request must not be null");
        }
        if (listener == null) {
            throw new IllegalArgumentException("listener must not be null");
        }

        byte[] payload = HttpIpcCodec.encodeRequest(request);
        if (payload == null) {
            throw new IOException("Unable to encode HTTP request");
        }

        int requestId;
        synchronized (this) {
            requestId = allocateRequestId();
            pending.put(Integer.valueOf(requestId), listener);
        }

        NativeMessage message = new NativeMessage(
            NativeMessage.TYPE_HTTP_REQUEST, requestId, payload);
        try {
            supervisor.sendMessage(message);
        } catch (IOException error) {
            HttpResponseListener removed;
            synchronized (this) {
                removed = (HttpResponseListener) pending.remove(
                    Integer.valueOf(requestId));
            }
            if (removed != null) {
                removed.onHttpError(requestId, error.getMessage());
            }
            throw error;
        }
        return requestId;
    }

    private int allocateRequestId() throws IOException {
        int candidate = nextRequestId;
        while (pending.containsKey(Integer.valueOf(candidate))) {
            candidate++;
            if (candidate <= 0) {
                candidate = 1;
            }
            if (candidate == nextRequestId) {
                throw new IOException("No request IDs available");
            }
        }
        nextRequestId = candidate + 1;
        if (nextRequestId <= 0) {
            nextRequestId = 1;
        }
        return candidate;
    }

    /**
     * Receives a native frame and dispatches it to the matching callback.
     */
    public void onNativeMessage(NativeMessage message) {
        if (message == null) {
            return;
        }
        if (message.getType() == NativeMessage.TYPE_HTTP_RESPONSE) {
            dispatchHttpResponse(message);
            return;
        }
        if (applicationListener != null) {
            applicationListener.onMessageReceived(message);
        }
    }

    private void dispatchHttpResponse(NativeMessage message) {
        HttpResponseListener listener;
        int requestId = message.getRequestId();
        synchronized (this) {
            listener = (HttpResponseListener) pending.remove(
                Integer.valueOf(requestId));
        }
        if (listener == null) {
            return;
        }

        HttpResponse response = HttpIpcCodec.decodeResponse(message.getPayload());
        if (response == null) {
            listener.onHttpError(requestId, "Malformed HTTP_RESPONSE payload");
            return;
        }
        listener.onHttpResponse(requestId, response);
    }

    /**
     * Notifies all outstanding requests that the native process terminated.
     */
    public void onNativeProcessTerminated(int exitCode) {
        Hashtable outstanding;
        synchronized (this) {
            outstanding = (Hashtable) pending.clone();
            pending.clear();
        }

        Enumeration keys = outstanding.keys();
        while (keys.hasMoreElements()) {
            Integer requestId = (Integer) keys.nextElement();
            HttpResponseListener listener = (HttpResponseListener) outstanding.get(requestId);
            listener.onHttpError(requestId.intValue(),
                "Native process terminated with exit code " + exitCode);
        }
        if (applicationListener != null) {
            applicationListener.onProcessTerminated(exitCode);
        }
    }

    public void onMessageReceived(NativeMessage message) {
        onNativeMessage(message);
    }

    public void onProcessTerminated(int exitCode) {
        onNativeProcessTerminated(exitCode);
    }
}
