#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct LinkedSelectionNode {
  unsigned char* string;
  uint64_t stringSize;

  struct LinkedSelectionNode* next;
  struct LinkedSelectionNode* previous;
};

struct LinkedSelectionNode* createLinkedSelectionNode() {
  struct LinkedSelectionNode* linkedSelectionNode;

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

  window = XCreateSimpleWindow(display, rootWindow, -10, -10, 1, 1, 0, 0, 0);
  property = XInternAtom(display, "PENGUIN", False);
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
      uint64_t size;
      uint64_t itemCount;
      unsigned char* propertyBuffer = NULL;

      XGetWindowProperty(display, window, property, 0, 0, False, AnyPropertyType, &type, &format, &itemCount, &size, &propertyBuffer);
      XFree(propertyBuffer);
      XGetWindowProperty(display, window, property, 0, size, False, AnyPropertyType, &type, &format, &itemCount, &itemCount, &propertyBuffer);

      linkedSelectionNode = (struct LinkedSelectionNode*)malloc(size * sizeof(unsigned char) + sizeof(uint64_t) + 2 * sizeof(struct LinkedSelectionNode*));
      linkedSelectionNode->string = (unsigned char*)malloc(size * sizeof(unsigned char));
      memcpy(linkedSelectionNode->string, propertyBuffer, size * sizeof(unsigned char));
      linkedSelectionNode->stringSize = size;

      XFree(propertyBuffer);

      XDeleteProperty(display, window, property);
    }
  }

  return linkedSelectionNode;
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
  int copyKeyCode = XKeysymToKeycode(display, XK_C);
  int pasteKeyCode = XKeysymToKeycode(display, XK_V);
  int closeKeyCode = XKeysymToKeycode(display, XK_W);

  XGrabKey(display, copyKeyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, pasteKeyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, closeKeyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XSelectInput(display, rootWindow, KeyPressMask);

  struct LinkedSelectionNode* rootNode = NULL;
  struct LinkedSelectionNode* currentNode = NULL;

  int isRunning = 1;
  while (isRunning) {
    XNextEvent(display, &event);
    if (event.type == KeyPress) {
      if (event.xkey.keycode == 54) {
        if (rootNode == NULL) {
          rootNode = createLinkedSelectionNode();
          currentNode = rootNode;
        }
        else {
          currentNode->next = createLinkedSelectionNode();
          currentNode = currentNode->next;
        }

        createWindow();
      }

      if (event.xkey.keycode == 55) {
        createWindow();
      }

      if (event.xkey.keycode == 25) {
        isRunning = 0;
      }
    }
  }

  XCloseDisplay(display);
  return 0;
}