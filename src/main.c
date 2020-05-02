// source: https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html

#include <stdio.h>
#include <X11/Xlib.h>

void show_utf8_prop(Display* display, Window window, Atom property) {
  Atom da, incr, type;
  int di;
  unsigned long size, dul;
  unsigned char* prop_ret = NULL;

  /* Dummy call to get type and size. */
  XGetWindowProperty(display, window, property, 0, 0, False, AnyPropertyType, &type, &di, &dul, &size, &prop_ret);
  XFree(prop_ret);

  incr = XInternAtom(display, "INCR", False);
  if (type == incr) {
    printf("Data too large and INCR mechanism not implemented\n");
    return;
  }

  /* Read the data in one go. */
  printf("Property size: %lu\n", size);

  XGetWindowProperty(display, window, property, 0, size, False, AnyPropertyType, &da, &di, &dul, &dul, &prop_ret);
  printf("%s\n", prop_ret);
  fflush(stdout);
  XFree(prop_ret);

  /* Signal the selection owner that we have successfully read the
   * data. */
  XDeleteProperty(display, window, property);
}

int main(void) {
  Display* display;
  Window target_window, root;
  int screen;
  Atom sel, target_property, utf8;
  XEvent ev;
  XSelectionEvent* sev;

  display = XOpenDisplay(NULL);
  screen = DefaultScreen(display);
  root = RootWindow(display, screen);

  sel = XInternAtom(display, "CLIPBOARD", False);
  utf8 = XInternAtom(display, "UTF8_STRING", False);

  /* The selection owner will store the data in a property on this
   * window: */
  target_window = XCreateSimpleWindow(display, root, -10, -10, 1, 1, 0, 0, 0);

  /* That's the property used by the owner. Note that it's completely
   * arbitrary. */
  target_property = XInternAtom(display, "PENGUIN", False);

  /* Request conversion to UTF-8. Not all owners will be able to
   * fulfill that request. */
  XConvertSelection(display, sel, utf8, target_property, target_window, CurrentTime);

  XNextEvent(display, &ev);
  if (ev.type == SelectionNotify) {
    sev = (XSelectionEvent*)&ev.xselection;
    if (sev->property == None) {
      printf("Conversion could not be performed.\n");
    }
    else {
      show_utf8_prop(display, target_window, target_property);
    }
  }

  return 0;
}