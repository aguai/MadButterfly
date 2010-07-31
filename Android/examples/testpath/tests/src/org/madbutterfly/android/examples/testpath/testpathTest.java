package org.madbutterfly.android.examples.testpath;

import android.test.ActivityInstrumentationTestCase2;

/**
 * This is a simple framework for a test of an Application.  See
 * {@link android.test.ApplicationTestCase ApplicationTestCase} for more information on
 * how to write and extend Application tests.
 * <p/>
 * To run this test, you can type:
 * adb shell am instrument -w \
 * -e class org.madbutterfly.android.examples.testpath.testpathTest \
 * org.madbutterfly.android.examples.testpath.tests/android.test.InstrumentationTestRunner
 */
public class testpathTest extends ActivityInstrumentationTestCase2<testpath> {

    public testpathTest() {
        super("org.madbutterfly.android.examples.testpath", testpath.class);
    }

}
