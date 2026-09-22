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
}
