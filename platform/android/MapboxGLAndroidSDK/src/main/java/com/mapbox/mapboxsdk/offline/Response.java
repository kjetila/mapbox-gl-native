package com.mapbox.mapboxsdk.offline;

/**
 * Created by kjetilandersen on 22.12.16.
 */

public class Response {
    //
    // Static methods
    //

    static {
        System.loadLibrary("mapbox-gl");
    }

    //Constructor
    private Response() {
        //For JNI use only
    }

}
