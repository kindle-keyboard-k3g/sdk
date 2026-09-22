package com.amazon.kindle.bridge.network;

import com.amazon.kindle.bridge.NativeMessage;
import java.io.IOException;
import java.io.OutputStream;

/**
 * Handles incoming TYPE_HTTP_REQUEST IPC frames from a native C++ process,
 * proxies them via WhispernetHttpClient, and writes TYPE_HTTP_RESPONSE frames
 * back to the process stdin.
 *
 * Thread safety: handleMessage() is called from the supervisor's reader thread.
 * The proxy calls are blocking; keep one relay per daemon process or run
 * dispatching in a dedicated thread pool.
 *
 * Compatible with Java 1.4 / CDC 1.1.
 */
public class NetworkRelayHandler {

    private final WhispernetHttpClient httpClient;
    private final OutputStream processStdin;
    private final HttpResponseListener listener;

    /**
     * Constructs a relay handler.
     *
     * @param httpClient   Whispernet HTTP client to use for outbound requests
     * @param processStdin stdin stream of the native process to reply to
     * @param listener     optional callback for responses (may be null)
     */
    public NetworkRelayHandler(WhispernetHttpClient httpClient,
                               OutputStream processStdin,
                               HttpResponseListener listener) {
        if (httpClient == null) {
            throw new IllegalArgumentException("httpClient must not be null");
        }
        if (processStdin == null) {
            throw new IllegalArgumentException("processStdin must not be null");
        }
        this.httpClient   = httpClient;
        this.processStdin = processStdin;
        this.listener     = listener;
    }

    /**
     * Processes a received IPC message. Only TYPE_HTTP_REQUEST messages are
     * handled; all other types are silently ignored.
     *
     * @param message incoming IPC message
     */
    public void handleMessage(NativeMessage message) {
        if (message.getType() != NativeMessage.TYPE_HTTP_REQUEST) {
            return;
        }
        int requestId = message.getRequestId();
        HttpRequest request = HttpIpcCodec.decodeRequest(message.getPayload());
        if (request == null) {
            sendErrorResponse(requestId, "Malformed HTTP_REQUEST payload");
            return;
        }
        HttpResponse response = httpClient.execute(request);
        sendResponse(requestId, response);
    }

    private void sendResponse(int requestId, HttpResponse response) {
        byte[] payload = HttpIpcCodec.encodeResponse(response);
        if (payload == null) {
            sendErrorResponse(requestId, "Failed to encode HTTP_RESPONSE");
            return;
        }
        NativeMessage reply = new NativeMessage(NativeMessage.TYPE_HTTP_RESPONSE,
                                                requestId, payload);
        try {
            synchronized (processStdin) {
                reply.writeTo(processStdin);
            }
        } catch (IOException e) {
            // Process is gone; nothing more to do
        }
        if (listener != null) {
            listener.onHttpResponse(requestId, response);
        }
    }

    private void sendErrorResponse(int requestId, String error) {
        sendResponse(requestId, new HttpResponse(error));
    }
}
