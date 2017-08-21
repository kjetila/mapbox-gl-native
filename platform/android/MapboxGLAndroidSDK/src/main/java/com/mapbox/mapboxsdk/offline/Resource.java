package com.mapbox.mapboxsdk.offline;

/**
 * Created by kjetilandersen on 22.12.16.
 */

public class Resource {
    //
    // Static methods
    //

    static {
        System.loadLibrary("mapbox-gl");
    }

    //Constructor
    private Resource() {
        //For JNI use only
    }

    public Resource(String urlTemplate, float pixelRatio, int x, int y, int z) {

    }
}