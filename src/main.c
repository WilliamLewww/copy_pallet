#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

unsigned char* clipboardString = NULL;

struct LinkedSelectionNode {
  unsigned char* string;
  uint64_t stringSize;

  struct LinkedSelectionNode* next;
  struct LinkedSelectionNode* previous;
};

struct WindowHints {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long inputMode;
  unsigned long status;
};

void sendSelectionEmpty(Display* display, XSelectionRequestEvent* selectionRequestEvent) {
  XSelectionEvent selectionEvent;

  selectionEvent.type = SelectionNotify;
  selectionEvent.requestor = selectionRequestEvent->requestor;
  selectionEvent.selection = selectionRequestEvent->selection;
  selectionEvent.target = selectionRequestEvent->target;
  selectionEvent.property = None;
  selectionEvent.time = selectionRequestEvent->time;

  XSendEvent(display, selectionRequestEvent->requestor, True, NoEventMask, (XEvent*)&selectionEvent);
}

void sendSelectionUTF8(Display* display, XSelectionRequestEvent* selectionRequestEvent, Atom utf8, char* data) {
    XSelectionEvent selectionEvent;

    XChangeProperty(display, selectionRequestEvent->requestor, selectionRequestEvent->property, utf8, 8, PropModeReplace, (unsigned char*)data, strlen(data));

    selectionEvent.type = SelectionNotify;
    selectionEvent.requestor = selectionRequestEvent->requestor;
    selectionEvent.selection = selectionRequestEvent->selection;
    selectionEvent.target = selectionRequestEvent->target;
    selectionEvent.property = selectionRequestEvent->property;
    selectionEvent.time = selectionRequestEvent->time;

    XSendEvent(display, selectionRequestEvent->requestor, True, NoEventMask, (XEvent*)&selectionEvent);
}

struct LinkedSelectionNode* createLinkedSelectionNode() {
  struct LinkedSelectionNode* linkedSelectionNode;

  Window window;
  Display* display = XOpenDisplay(NULL);
  Window rootWindow = RootWindow(display, DefaultScreen(display));

  XEvent event;
  XSelectionEvent* selectionEvent;

  Atom selection = XInternAtom(display, "CLIPBOARD", False);
  Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
  Atom property = XInternAtom(display, "PROPERTY", False);

  window = XCreateSimpleWindow(display, rootWindow, -10, -10, 1, 1, 0, 0, 0);
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
      linkedSelectionNode->next = NULL;
      linkedSelectionNode->previous = NULL;

      XFree(propertyBuffer);

      XDeleteProperty(display, window, property);
    }
  }

  XDestroyWindow(display, window);
  XCloseDisplay(display);
  return linkedSelectionNode;
}

void getMousePosition(int* mouseX, int* mouseY) {
  Window* rootWindow;
  Window returnWindow;

  int numberOfScreen;
  int rootX;
  int rootY;
  int windowX;
  int windowY;
  unsigned int maskReturn;

  Display* display = XOpenDisplay(NULL);
  numberOfScreen = XScreenCount(display);
  rootWindow = malloc(numberOfScreen * sizeof(Window));

  for (int x = 0; x < numberOfScreen; x++) {
    rootWindow[x] = XRootWindow(display, x);
  }
  for (int x = 0; x < numberOfScreen; x++) {
    XQueryPointer(display, rootWindow[x], &returnWindow, &returnWindow, &rootX, &rootY, &windowX, &windowY, &maskReturn);
  }

  *mouseX = rootX;
  *mouseY = rootY;

  free(rootWindow);
  XCloseDisplay(display);
}

void createSelectionWindow(struct LinkedSelectionNode* currentNode) {
  Display* display;
  Window window;
  XEvent event;
  int screen;

  XSizeHints* sizeHints;

  display = XOpenDisplay(NULL);
  screen = DefaultScreen(display);

  int mouseX = 0;
  int mouseY = 0;
  getMousePosition(&mouseX, &mouseY);

  window = XCreateSimpleWindow(display, RootWindow(display, screen), mouseX, mouseY, 200, 115, 1, BlackPixel(display, screen), WhitePixel(display, screen));
  
  sizeHints = XAllocSizeHints();
  sizeHints->flags=USPosition | PAspect | PMinSize | PMaxSize;
  sizeHints->min_width=200;
  sizeHints->min_height=115;
  sizeHints->max_width=200*2;
  sizeHints->max_height=115;
  XSetWMNormalHints(display, window, sizeHints);

  struct WindowHints windowHints;
  Atom property;
  windowHints.flags = 2;
  windowHints.decorations = 0;
  property = XInternAtom(display, "_MOTIF_WM_HINTS", True);
  XChangeProperty(display, window, property, property, 32, PropModeReplace, (unsigned char*)&windowHints, 5);

  XSelectInput(display, window, ExposureMask | KeyPressMask | FocusChangeMask);
  XMapWindow(display, window);

  int focusCount = 0;
  int currentPage = 0;

  int snippetCount = 6;

  unsigned char* selectedString = NULL;

  int isRunning = 1;
  while (isRunning) {
    XNextEvent(display, &event);
    if (event.type == Expose) {
      XDrawRectangle(display, window, DefaultGC(display, screen), 3, 18, 193, 93);

      char* pageBuffer = (char*)malloc(7 * sizeof(char));
      strcpy(pageBuffer, "Page: ");
      pageBuffer[6] = (currentPage + 1) + '0';
      XDrawString(display, window, DefaultGC(display, screen), 5, 15, pageBuffer, strlen(pageBuffer));
      free(pageBuffer);

      struct LinkedSelectionNode* tempCurrentNode = currentNode;
      for (int x = 0; x < snippetCount; x++) {
        if (tempCurrentNode != NULL) {
          char* indexBuffer = (char*)malloc(2 * sizeof(char));
          indexBuffer[0] = (x + 1) + '0';
          indexBuffer[1] = ':';

          XDrawString(display, window, DefaultGC(display, screen), 5, 31 + 15 * x, indexBuffer, 2);
          XDrawString(display, window, DefaultGC(display, screen), 23, 31 + 15 * x, (char*)tempCurrentNode->string, 28 > tempCurrentNode->stringSize ? tempCurrentNode->stringSize : 28);
          tempCurrentNode = tempCurrentNode->previous;

          free(indexBuffer);
        }
      }
    }
    if (event.type == FocusOut) {
      if (focusCount >= 2) {
        isRunning = 0;
      }
      focusCount += 1;
    }
    if (event.type == FocusIn) {
      focusCount += 1;
    }
    if (event.type == KeyPress) {
      int keyIndex = (event.xkey.keycode - 9) + (snippetCount * currentPage);

      if (event.xkey.keycode - 9 > 0 && event.xkey.keycode - 9 < 7) {
        struct LinkedSelectionNode* tempCurrentNode = currentNode;
        int count = 0;
        while (tempCurrentNode != NULL && count < keyIndex - 1) {
          tempCurrentNode = tempCurrentNode->previous;
          count += 1;
        }

        if (count == keyIndex - 1 && tempCurrentNode != NULL) {
          isRunning = 0;
          selectedString = tempCurrentNode->string;
        }
      }

      if (event.xkey.keycode == XKeysymToKeycode(display, XK_W)) {
        if (currentPage > 0) {
          currentPage -= 1;
        }

        XClearWindow(display, window);
        XDrawRectangle(display, window, DefaultGC(display, screen), 3, 18, 193, 93);

        char* pageBuffer = (char*)malloc(7 * sizeof(char));
        strcpy(pageBuffer, "Page: ");
        pageBuffer[6] = (currentPage + 1) + '0';
        XDrawString(display, window, DefaultGC(display, screen), 5, 15, pageBuffer, strlen(pageBuffer));
        free(pageBuffer);

        struct LinkedSelectionNode* tempCurrentNode = currentNode;
        for (int x = 0; x < snippetCount * currentPage; x++) {
          if (tempCurrentNode != NULL) {
            tempCurrentNode = tempCurrentNode->previous;
          }
        }
        for (int x = 0; x < snippetCount; x++) {
          if (tempCurrentNode != NULL) {
            char* indexBuffer = (char*)malloc(2 * sizeof(char));
            indexBuffer[0] = (x + 1) + '0';
            indexBuffer[1] = ':';

            XDrawString(display, window, DefaultGC(display, screen), 5, 31 + 15 * x, indexBuffer, 2);
            XDrawString(display, window, DefaultGC(display, screen), 23, 31 + 15 * x, (char*)tempCurrentNode->string, 28 > tempCurrentNode->stringSize ? tempCurrentNode->stringSize : 28);
            tempCurrentNode = tempCurrentNode->previous;

            free(indexBuffer);
          }
        }
      }
      if (event.xkey.keycode == XKeysymToKeycode(display, XK_S)) {
        struct LinkedSelectionNode* tempCurrentNode = currentNode;
        for (int x = 0; x < snippetCount * (currentPage + 1); x++) {
          if (tempCurrentNode != NULL) {
            tempCurrentNode = tempCurrentNode->previous;
          }
        }

        if (tempCurrentNode != NULL) {
          currentPage += 1;
        }
      
        XClearWindow(display, window);
        XDrawRectangle(display, window, DefaultGC(display, screen), 3, 18, 193, 93);
        
        char* pageBuffer = (char*)malloc(7 * sizeof(char));
        strcpy(pageBuffer, "Page: ");
        pageBuffer[6] = (currentPage + 1) + '0';
        XDrawString(display, window, DefaultGC(display, screen), 5, 15, pageBuffer, strlen(pageBuffer));
        free(pageBuffer);

        tempCurrentNode = currentNode;
        for (int x = 0; x < snippetCount * currentPage; x++) {
          if (tempCurrentNode != NULL) {
            tempCurrentNode = tempCurrentNode->previous;
          }
        }
        for (int x = 0; x < snippetCount; x++) {
          if (tempCurrentNode != NULL) {
            char* indexBuffer = (char*)malloc(2 * sizeof(char));
            indexBuffer[0] = (x + 1) + '0';
            indexBuffer[1] = ':';

            XDrawString(display, window, DefaultGC(display, screen), 5, 31 + 15 * x, indexBuffer, 2);
            XDrawString(display, window, DefaultGC(display, screen), 23, 31 + 15 * x, (char*)tempCurrentNode->string, 28 > tempCurrentNode->stringSize ? tempCurrentNode->stringSize : 28);
            tempCurrentNode = tempCurrentNode->previous;

            free(indexBuffer);
          }
        }
      }

      if (event.xkey.keycode == XKeysymToKeycode(display, XK_Escape)) {
        isRunning = 0;
      }
    }
  }
  if (selectedString != NULL) {
    clipboardString = selectedString;
  }

  XDestroyWindow(display, window);
  XCloseDisplay(display);
}

int main(void) {
  Display* display = XOpenDisplay(0);
  Window rootWindow = DefaultRootWindow(display);

  XEvent event;
  XSelectionRequestEvent* selectionRequestEvent;

  unsigned int modifiers = ControlMask | ShiftMask;
  int copyKeyCode = XKeysymToKeycode(display, XK_C);
  int pasteKeyCode = XKeysymToKeycode(display, XK_V);
  int closeKeyCode = XKeysymToKeycode(display, XK_W);

  XGrabKey(display, copyKeyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, pasteKeyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, closeKeyCode, modifiers, rootWindow, False, GrabModeAsync, GrabModeAsync);
  XSelectInput(display, rootWindow, KeyPressMask);

  Atom selection;
  Atom utf8;

  selection = XInternAtom(display, "CLIPBOARD", False);
  utf8 = XInternAtom(display, "UTF8_STRING", False);

  struct LinkedSelectionNode* rootNode = NULL;
  struct LinkedSelectionNode* currentNode = NULL;

  int isRunning = 1;
  while (isRunning) {
    XNextEvent(display, &event);

    if (event.type == SelectionClear) {

    }
    if (event.type == SelectionRequest) {
      selectionRequestEvent = (XSelectionRequestEvent*)&event.xselectionrequest;

      if (selectionRequestEvent->target != utf8 || selectionRequestEvent->property == None) {
        sendSelectionEmpty(display, selectionRequestEvent);
      }
      else {
        sendSelectionUTF8(display, selectionRequestEvent, utf8, (char*)clipboardString);
      }
    } 

    if (event.type == KeyPress) {
      if (event.xkey.keycode == XKeysymToKeycode(display, XK_C)) {
        if (rootNode == NULL) {
          rootNode = createLinkedSelectionNode();
          currentNode = rootNode;
        }
        else {
          currentNode->next = createLinkedSelectionNode();
          currentNode->next->previous = currentNode;
          currentNode = currentNode->next;
        }
      }

      if (event.xkey.keycode == XKeysymToKeycode(display, XK_V)) {
        XSetSelectionOwner(display, selection, rootWindow, CurrentTime);
        createSelectionWindow(currentNode);
      }

      if (event.xkey.keycode == XKeysymToKeycode(display, XK_W)) {
        isRunning = 0;
      }
    }
  }

  XCloseDisplay(display);
  return 0;
}