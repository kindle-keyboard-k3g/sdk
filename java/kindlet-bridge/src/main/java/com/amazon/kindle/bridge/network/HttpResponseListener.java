package com.amazon.kindle.bridge.network;

/**
 * Callback interface for asynchronous HTTP relay responses.
 * Compatible with Java 1.4 / CDC 1.1.
 */
public interface HttpResponseListener {

    /**
     * Called when a proxied HTTP response is ready.
     *
     * @param requestId original request correlation ID
     * @param response  HTTP response, or a transport-error response on failure
     */
    void onHttpResponse(int requestId, HttpResponse response);

    /**
     * Called when the native process cannot complete a pending HTTP request.
     *
     * @param requestId original request correlation ID
     * @param message human-readable error message
     */
    void onHttpError(int requestId, String message);
}
