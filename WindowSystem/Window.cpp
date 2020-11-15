#include "Window.hpp"

#include <stdlib.h>

// TODO: Should probably split this into even more files, as it becomes increasingly difficult to navigate and maintain

// AbstractWindow methods

constexpr unsigned int scrollbarElemSize = 25;

void AbstractWindow::draw() {
}

AbstractWindow::AbstractWindow() {
    parent = nullptr;
    eventMask = 0;
}

AbstractWindow::~AbstractWindow() {
    if (parent) {
        // parent->detach(this);
        printf("%p is dying and detaching from parent %p\n", static_cast<void*>(this), static_cast<void*>(parent));
    } else {
        printf("Orphan window %p is dying\n", static_cast<void*>(this));
    }
}

void AbstractWindow::processEvent(Event ev) {
    if (!(ev.eventType & eventMask))
        return;

    handleEvent(ev);
}

void AbstractWindow::attachToParent(AbstractWindow* parent) {
    this->parent = parent;
    parent->updateEventMask(eventMask);
}

void AbstractWindow::updateEventMask(uint64_t update) {
    eventMask |= update;
    if (parent) {
        parent->updateEventMask(update);
    }
}

void AbstractWindow::handleEvent(Event ev) {
    printf("Event type: %lx\nEvent type mask: %lx\n\n", ev.eventType, eventMask);
}

// ContainerWindow methods

ContainerWindow::~ContainerWindow() {
    for (auto child : children) {
        delete child;
    }
}

void ContainerWindow::draw() {
    for (auto child : children) {
        child->draw();
    }
}

void ContainerWindow::processEvent(Event ev) {
    if (!(ev.eventType & eventMask))
        return;

    for (auto child : children) {
        child->processEvent(ev);
    }

    handleEvent(ev);
}

void ContainerWindow::attachChild(AbstractWindow* win) {
    children.push_back(win);
    win->attachToParent(this);
}

// AbstractButton methods
void AbstractButton::attachToParent(AbstractWindow* parent) {
    this->parent = parent;
    updateEventMask(EV_MOUSE_KEY_PRESS | EV_MOUSE_KEY_RELEASE | EV_MOUSE_MOVE);  // Need to track key presses, releases & movements in order to properly update state
}

AbstractButton::AbstractButton() {
    pressed = false;
    hovered = false;
}

void AbstractButton::handleEvent(Event ev) {
    if (IS_MOUSE_EV(ev)) {
        bool inside = isInside(ev.mouse.x, ev.mouse.y);

        if (ev.eventType == EV_MOUSE_KEY_PRESS && inside) {
            hovered = true;
            if (!pressed) {
                pressed = true;
                onButtonPress(ev);
            }
        } else if (ev.eventType == EV_MOUSE_KEY_RELEASE && pressed) {
            onButtonRelease(ev);
            if (inside) click(ev);
            pressed = false;
        } else if (ev.eventType == EV_MOUSE_MOVE) {
            if (inside) {
                if (!hovered) {
                    hovered = true;
                    onHoverEnter(ev);
                }
            } else {
                if (hovered) {
                    hovered = false;
                    onHoverExit(ev);
                }
            }
            onMouseMove(ev);
        }
    }
}

// Rectangle methods
void Rectangle::setPosition(int x, int y) {
    this->x = x;
    this->y = y;
}

void Rectangle::setSize(unsigned int width, unsigned int height) {
    this->width = width;
    this->height = height;
}

void Rectangle::setThickness(float thickness) {
    this->thickness = thickness;
}

void Rectangle::setBackgroundColor(const Color& color) {
    this->bkg = color;
}

void Rectangle::setOutlineColor(const Color& color) {
    this->frg = color;
}

bool Rectangle::isInsideRect(int x, int y) {
    int xstart = this->x;
    int xend = this->x + this->width;
    int ystart = this->y;
    int yend = this->y + this->height;

    if (thickness > 0) {
        xstart -= thickness;
        ystart -= thickness;
        xend += thickness;
        yend += thickness;
    }

    return (xstart <= x) && (ystart <= y) && (xend >= x) && (yend >= y);
}

int Rectangle::getWidth() {
    return width;
}

int Rectangle::getHeight() {
    return height;
}

// RectangleButton methods
void RectangleButton::draw() {
    RenderEngine::DrawRect(x, y, width, height, bkg, frg, thickness);
}

void RectangleWindow::draw() {
    RenderEngine::DrawRect(x, y, width, height, bkg, frg, thickness);
}

void RectangleButton::setHoverColor(const Color& color) {
    hoverBkg = color;
}

void RectangleButton::setPressColor(const Color& color) {
    pressBkg = color;
}

void RectangleButton::click(const Event&) {
    printf("Abstract button %p just got clicked\n", static_cast<void*>(this));
}

void RectangleButton::onHoverEnter(const Event&) {
    bkg = hoverBkg;
}

void RectangleButton::onHoverExit(const Event&) {
    bkg = defaultBkg;
}

void RectangleButton::onButtonPress(const Event&) {
    bkg = pressBkg;
}

void RectangleButton::setBackgroundColor(const Color& color) {
    defaultBkg = color;
    bkg = color;
}

void RectangleButton::onButtonRelease(const Event&) {}

void RectangleButton::onMouseMove(const Event&) {}

bool RectangleButton::isInside(int x, int y) {
    return Rectangle::isInsideRect(x, y);
}

// Slider methods
void Slider::setPosition(int x, int y) {
    this->x = x;
    this->y = y;

    if (isHorizontal) {
        pivot = x;
    } else {
        pivot = y;
    }
}

Slider::Slider(bool isHorizontal) : isHorizontal(isHorizontal) {
    eventMask |= EV_SCROLL;  // Don't want to propagate subscription
}

void Slider::setLimit(int limit) {
    this->limit = limit;
}

void Slider::onButtonPress(const Event& ev) {
    if (isHorizontal) {
        strokeStart = ev.mouse.x;
        movementStart = x;
    } else {
        strokeStart = ev.mouse.y;
        movementStart = y;
    }
}

void Slider::onButtonRelease(const Event&) {
}

void Slider::onMouseMove(const Event& ev) {
    if (pressed) {
        if (isHorizontal) {
            x = movementStart + ev.mouse.x - strokeStart;
        } else {
            y = movementStart + ev.mouse.y - strokeStart;
        }
    }
}

void Slider::handleEvent(Event ev) {
    if (IS_MOUSE_EV(ev)) {
        AbstractButton::handleEvent(ev);
    } else if (ev.eventType == EV_SCROLL) {
        switch (ev.scroll.scrollType) {
            case Event::UP:
                if (isHorizontal) {
                    x -= 10;
                } else {
                    y -= 10;
                }
                break;

            case Event::DOWN:
                if (isHorizontal) {
                    x += 10;
                } else {
                    y += 10;
                }
                break;

            case Event::PG_UP:
                if (isHorizontal) {
                    x -= width;
                } else {
                    y -= height;
                }
                break;

            case Event::PG_DOWN:
                if (isHorizontal) {
                    x += width;
                } else {
                    y += height;
                }
                break;

            default:
                break;
        }
    }

    if (isHorizontal) {
        if (x < pivot)
            x = pivot;
        else if (x > pivot + limit)
            x = pivot + limit;
    } else {
        if (y < pivot)
            y = pivot;
        else if (y > pivot + limit)
            y = pivot + limit;
    }

    if (pressed) {
        bkg = pressBkg;
    } else if (hovered) {
        bkg = hoverBkg;
    } else {
        bkg = defaultBkg;
    }

    Event scrollEv;
    scrollEv.eventType = EV_SCROLL;

    Scrollbar* sParent = dynamic_cast<Scrollbar*>(parent);
    sParent->handleEvent(scrollEv);
    fprintf(stderr, "%p %p\n", static_cast<void*>(this), static_cast<void*>(sParent));
}

int Slider::getPositionAlongAxis() {
    if (isHorizontal) {
        return x;
    } else {
        return y;
    }
}

// Scrollbar methods
Scrollbar::Scrollbar(int length, bool isHorizontal) : isHorizontal(isHorizontal), length(length) {
    up = new ScrollbarButton(true);
    down = new ScrollbarButton(false);
    slider = new Slider(isHorizontal);
    bkg = new ScrollbarBackground(isHorizontal);

    attachChild(bkg);
    attachChild(up);
    attachChild(down);
    attachChild(slider);

    up->setSize(scrollbarElemSize, scrollbarElemSize);
    down->setSize(scrollbarElemSize, scrollbarElemSize);

    if (isHorizontal) {
        bkg->setSize(length, scrollbarElemSize);
    } else {
        bkg->setSize(scrollbarElemSize, length);
    }

    up->setThickness(-1);
    down->setThickness(-1);
    slider->setThickness(-1);
    bkg->setThickness(-1);

    eventMask |= EV_SCROLL;  // Don't really want to propagate subscription to scroll event since we want
}

void Scrollbar::handleEvent(Event ev) {
    if (!parent) return;
    fprintf(stderr, "Scrollbar %p processing event of type %lu\n", static_cast<void*>(this), ev.eventType);
    if (ev.eventType == EV_SCROLL) {
        fprintf(stderr, "THAT IS A GODDAMN SCROLL EVENT!\n");
        if (isHorizontal) {
            ev.scroll.isHorizontal = true;
            ev.scroll.position = ((float)slider->x - slider->pivot) / (slider->limit + slider->width);
            
        } else {
            ev.scroll.isHorizontal = false;
            ev.scroll.position = ((float)slider->y - slider->pivot) / (slider->limit + slider->height);
            fprintf(stderr, "Scroll pos: %f\n", ev.scroll.position);
        }

        parent->processEvent(ev);
    }
}

void Scrollbar::setLength(int length) {
    this->length = length;
    if (isHorizontal) {
        bkg->setSize(length, scrollbarElemSize);
    } else {
        bkg->setSize(scrollbarElemSize, length);
    }
    setPosition(x, y);
}

void Scrollbar::setSliderSize(int size) {
    if (isHorizontal) {
        slider->setSize(size, scrollbarElemSize);

    } else {
        slider->setSize(scrollbarElemSize, size);
    }

    slider->setLimit(length - size);
}

void Scrollbar::setPosition(int x, int y) {
    this->x = x;
    this->y = y;

    up->setPosition(x, y);
    if (isHorizontal) {
        bkg->setPosition(x + scrollbarElemSize, y);
        slider->setPosition(x + scrollbarElemSize, y);
        down->setPosition(x + scrollbarElemSize + length, y);
    } else {
        bkg->setPosition(x, y + scrollbarElemSize);
        slider->setPosition(x, y + scrollbarElemSize);
        down->setPosition(x, y + scrollbarElemSize + length);
    }
}

void Scrollbar::setOutlineColor(const Color& color) {
    up->setOutlineColor(color);
    down->setOutlineColor(color);
    slider->setOutlineColor(color);
    bkg->setOutlineColor(color);
}

void Scrollbar::setBackgroundColor(const Color& color) {
    up->setBackgroundColor(color);
    down->setBackgroundColor(color);
    slider->setBackgroundColor(color);
    bkg->setBackgroundColor(color);
}

void Scrollbar::setHoverColor(const Color& color) {
    up->setHoverColor(color);
    down->setHoverColor(color);
    slider->setHoverColor(color);
}

void Scrollbar::setPressColor(const Color& color) {
    up->setPressColor(color);
    down->setPressColor(color);
    slider->setPressColor(color);
}

bool Scrollbar::isInsideSlider(int x, int y) {
    return slider->isInside(x, y);
}

int Scrollbar::getSliderPositionAlongAxis() {
    return slider->getPositionAlongAxis();
}

int Scrollbar::getBkgLength() {
    if (isHorizontal) {
        return bkg->getWidth();
    } else {
        return bkg->getHeight();
    }
}

// Scrollbar button methods
ScrollbarButton::ScrollbarButton(bool isUp) : isUp(isUp) {}

void ScrollbarButton::click(const Event&) {
    if (!parent)
        return;

    Event scrollEvent;
    scrollEvent.eventType = EV_SCROLL;

    if (isUp) {
        scrollEvent.scroll.scrollType = Event::UP;
    } else {
        scrollEvent.scroll.scrollType = Event::DOWN;
    }

    parent->processEvent(scrollEvent);
}

// ScrollbarBackground methods
ScrollbarBackground::ScrollbarBackground(bool isHorizontal) : isHorizontal(isHorizontal) {
    eventMask = EV_MOUSE_KEY_RELEASE;
}

void ScrollbarBackground::handleEvent(Event ev) {
    Scrollbar* p = static_cast<Scrollbar*>(parent);  // Believe me
    if (!parent) return;

    if (ev.eventType == EV_MOUSE_KEY_RELEASE && isInsideRect(ev.mouse.x, ev.mouse.y) && !p->isInsideSlider(ev.mouse.x, ev.mouse.y)) {
        printf("Scrollbar background had received an interesting event!\n");
        int coord = 0;
        if (isHorizontal) {
            coord = ev.mouse.x;
        } else {
            coord = ev.mouse.y;
        }

        Event newEv;
        newEv.eventType = EV_SCROLL;

        if (coord < p->getSliderPositionAlongAxis()) {
            newEv.scroll.scrollType = Event::PG_UP;
        } else {
            newEv.scroll.scrollType = Event::PG_DOWN;
        }

        p->processEvent(newEv);
    }
}

// ScrollbarManager methods
ScrollbarManager::ScrollbarManager(bool horizontalScrollable, bool verticalScrollable) {
    eventMask |= EV_SCROLL;  // Want to process event; don't really want to propagate subscription since scroll events are going to be issued by a child
    if (horizontalScrollable) {
        horizontal = new Scrollbar(100, true);
        horizontal->attachToParent(this);
    }

    if (verticalScrollable) {
        vertical = new Scrollbar(100, false);
        vertical->attachToParent(this);
    }
}

void ScrollbarManager::processEvent(Event ev) {
    fprintf(stderr, "Issuing event of type %lu\n", ev.eventType);
    if (ev.eventType != EV_SCROLL) {
        if (horizontal) {
            horizontal->processEvent(ev);
        }

        if (vertical) {
            vertical->processEvent(ev);
        }
    }

    for (auto child : children) {
        child->processEvent(ev);
    }
}

// TODO Turn this goddamn scrollbar width into a parameter, not a fucking magic number

void ScrollbarManager::adjustScrollbarSize(int x, int y, int width, int height) {
    adjWidth = width;
    adjHeight = height;

    if (horizontal) {
        horizontal->setPosition(x, y + height);
        horizontal->setLength(width - 2 * scrollbarElemSize);
    }

    if (vertical) {
        vertical->setPosition(x + width, y);
        vertical->setLength(height - 2 * scrollbarElemSize);
    }
}

void ScrollbarManager::adjustScrollableAreaSize(int width, int height) {
    if (horizontal) {
        horizontal->setSliderSize(static_cast<double>(adjWidth) / (width) * horizontal->getBkgLength());
    }

    if (vertical) {
        vertical->setSliderSize(static_cast<double>(adjHeight) / (height) * vertical->getBkgLength());
    }
}

void ScrollbarManager::draw() {
    if (horizontal)
        horizontal->draw();

    if (vertical)
        vertical->draw();

    for (auto child : children) {
        child->draw();
    }
}

// TextWindow methods
TextWindow::TextWindow() {
    content = nullptr;
    viewX = 0;
    viewY = 0;
}

void TextWindow::setText(const wchar_t* newContent) {
    content = newContent;
}

void TextWindow::setViewportPosition(int x, int y) {
    viewX = x;
    viewY = y;
}

void TextWindow::setViewportSpan(int x, int y) {
    spanX = x;
    spanY = y;
}

void TextWindow::draw() {
    // printf("Drawing text \"%s\" at (%d, %d)\n", content, x, y);
    RenderEngine::InitOffScreen(width, height);
    RenderEngine::DrawText(-viewX, -viewY, content);
    RenderEngine::FlushOffScreen(x, y);
    RenderEngine::DrawRect(x, y, width, height, bkg, frg, thickness);
}

void TextWindow::handleEvent(Event ev) {
    if (ev.eventType == EV_SCROLL) {
        if (ev.scroll.isHorizontal) {
            viewX = ev.scroll.position * spanX;
        } else {
            viewY = ev.scroll.position * spanY;
        }
    }
}

// P. S.: it's a mess, destroy this abomination