// source: https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html

#include <stdio.h>
#include <X11/Xlib.h>

void show_utf8_prop(Display* display, Window window, Atom property) {
  Atom type;
  int format;
  unsigned long size, itemCount;
  unsigned char* propertyBuffer = NULL;

  XGetWindowProperty(display, window, property, 0, 0, False, AnyPropertyType, &type, &format, &itemCount, &size, &propertyBuffer);
  printf("Property size: %lu\n", size);
  XFree(propertyBuffer);

  XGetWindowProperty(display, window, property, 0, size, False, AnyPropertyType, &type, &format, &itemCount, &itemCount, &propertyBuffer);
  printf("%s\n", propertyBuffer);
  XFree(propertyBuffer);

  XDeleteProperty(display, window, property);
}

int main(void) {
  Display* display;
  Window targetWindow, root;
  int screen;
  Atom selection, targetProperty, utf8;
  XEvent event;
  XSelectionEvent* selectionEvent;

  display = XOpenDisplay(NULL);
  screen = DefaultScreen(display);
  root = RootWindow(display, screen);

  selection = XInternAtom(display, "CLIPBOARD", False);
  utf8 = XInternAtom(display, "UTF8_STRING", False);

  /* The selection owner will store the data in a property on this
   * window: */
  targetWindow = XCreateSimpleWindow(display, root, -10, -10, 1, 1, 0, 0, 0);

  /* That's the property used by the owner. Note that it's completely
   * arbitrary. */
  targetProperty = XInternAtom(display, "PENGUIN", False);

  /* Request conversion to UTF-8. Not all owners will be able to
   * fulfill that request. */
  XConvertSelection(display, selection, utf8, targetProperty, targetWindow, CurrentTime);

  XNextEvent(display, &event);
  if (event.type == SelectionNotify) {
    selectionEvent = (XSelectionEvent*)&event.xselection;
    if (selectionEvent->property == None) {
      printf("Conversion could not be performed.\n");
    }
    else {
      show_utf8_prop(display, targetWindow, targetProperty);
    }
  }

  return 0;
}