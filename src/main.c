#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

void getPropertyUTF8() {
  Display* display;
  Window window;
  Window rootWindow;
  int screen;

  Atom selection;
  Atom property;
  Atom utf8;

  XEvent event;
  XSelectionEvent* selectionEvent;

  display = XOpenDisplay(NULL);
  screen = DefaultScreen(display);
  rootWindow = RootWindow(display, screen);

  selection = XInternAtom(display, "CLIPBOARD", False);
  utf8 = XInternAtom(display, "UTF8_STRING", False);

  /* The selection owner will store the data in a property on this
   * window: */
  window = XCreateSimpleWindow(display, rootWindow, -10, -10, 1, 1, 0, 0, 0);

  /* That's the property used by the owner. Note that it's completely
   * arbitrary. */
  property = XInternAtom(display, "PENGUIN", False);

  /* Request conversion to UTF-8. Not all owners will be able to
   * fulfill that request. */
  XConvertSelection(display, selection, utf8, property, window, CurrentTime);

  XNextEvent(display, &event);
  if (event.type == SelectionNotify) {
    selectionEvent = (XSelectionEvent*)&event.xselection;
    if (selectionEvent->property == None) {
      printf("Conversion could not be performed.\n");
    }
    else {
      Atom type;
      int format;
      unsigned long size;
      unsigned long itemCount;
      unsigned char* propertyBuffer = NULL;

      XGetWindowProperty(display, window, property, 0, 0, False, AnyPropertyType, &type, &format, &itemCount, &size, &propertyBuffer);
      printf("Property size: %lu\n", size);
      XFree(propertyBuffer);

      XGetWindowProperty(display, window, property, 0, size, False, AnyPropertyType, &type, &format, &itemCount, &itemCount, &propertyBuffer);
      printf("%s\n", propertyBuffer);
      XFree(propertyBuffer);

      XDeleteProperty(display, window, property);
    }
  }
}

void createWindow() {
  Display* display;
  Window window;
  XEvent event;
  int screen;

  const char* msg = "Hello, World!";

  display = XOpenDisplay(NULL);
  screen = DefaultScreen(display);
  window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, 100, 100, 1, BlackPixel(display, screen), WhitePixel(display, screen));
  XSelectInput(display, window, ExposureMask | KeyPressMask);
  XMapWindow(display, window);

  while (1) {
    XNextEvent(display, &event);
    if (event.type == Expose) {
      XFillRectangle(display, window, DefaultGC(display, screen), 20, 20, 10, 10);
      XDrawString(display, window, DefaultGC(display, screen), 10, 50, msg, strlen(msg));
    }
    if (event.type == KeyPress) {
      break;
    }
  }
  XCloseDisplay(display);
}

int main(void) {
  Display* display = XOpenDisplay(0);
  Window rootWindow = DefaultRootWindow(display);
  XEvent event;

  unsigned int modifiers = ControlMask | ShiftMask;
  int keycode = XKeysymToKeycode(display,XK_K);
  Window grabWindow = rootWindow;
  Bool ownerEvent = False;
  int pointerMode = GrabModeAsync;
  int keyboardMode = GrabModeAsync;

  XGrabKey(display, keycode, modifiers, grabWindow, ownerEvent, pointerMode, keyboardMode);

  XSelectInput(display, rootWindow, KeyPressMask);
  while(1) {
    XNextEvent(display, &event);
    if (event.type == KeyPress) {
      // XUngrabKey(display,keycode,modifiers,grabWindow);
      createWindow();
    }
  }

  XCloseDisplay(display);
  return 0;
}