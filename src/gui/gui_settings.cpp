/*  gui_settings.cpp
 *
 *  (c) 2011-2012 Anton Olkhovik <ant007h@gmail.com>
 *
 *  This file is part of Mokomaze - labyrinth game.
 *
 *  Mokomaze is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mokomaze is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mokomaze.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <SDL/SDL.h>
#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include "gui_font.h"
#include "gui_msgbox.h"
#include "gui_settings.h"
#include "../fonts.h"
#include "../mazecore/mazehelpers.h"

#define FIX_FOCUSHANDLER_EXCEPTION
#define FIX_TAB_SCROLL
#define FIX_DROPDOWN_HEIGHT

// Issue 59: sometimes dropdown halts applications when dropdown is clicked
// http://code.google.com/p/guichan/issues/detail?id=59
class SuperGui : public gcn::Gui
{
#ifdef FIX_FOCUSHANDLER_EXCEPTION
protected:
    void handleModalFocusGained()
    {
        // Distribute an event to all widgets in the "widget with mouse" queue.
        while (!mWidgetWithMouseQueue.empty())
        {
            gcn::Widget* widget = mWidgetWithMouseQueue.front();

            if (gcn::Widget::widgetExists(widget) && widget->_getFocusHandler())
            {
                distributeMouseEvent(widget,
                                     gcn::MouseEvent::EXITED,
                                     mLastMousePressButton,
                                     mLastMouseX,
                                     mLastMouseY,
                                     true,
                                     true);
            }

            mWidgetWithMouseQueue.pop_front();
        }

        mFocusHandler->setLastWidgetWithModalMouseInputFocus(mFocusHandler->getModalMouseInputFocused());
    }
#endif
};

// Issue 137: TabbedArea does not call the logic()-method of its children
// http://code.google.com/p/guichan/issues/detail?id=137
class SuperTabbedArea : public gcn::TabbedArea
{
#ifdef FIX_TAB_SCROLL
    void logic()
    {
        for (unsigned int i = 0; i < mTabs.size(); i++)
        {
            mTabs[i].second->logic();
        }
    }
#endif
};

// Common stuff
static bool running = false;
static User *user_set = NULL;
static User *user_set_new = NULL;

// SDL stuff
static SDL_Surface *_screen = NULL;
static SDL_Event event;

// Guichan SDL stuff
gcn::SDLInput *input = NULL;             // Input driver
gcn::SDLGraphics *graphics = NULL;       // Graphics driver
gcn::SDLImageLoader *imageLoader = NULL; // For loading images

// Guichan stuff
SuperGui *gui = NULL;       // GUI object - binds it all together
gcn::Container *top = NULL; // Top container
Font *font = NULL;          // Fonts
Font *titleFont = NULL;
Font *infoFont = NULL;

// Return values
bool calibration_requested = false;
bool video_set_modified = false;
bool input_set_modified = false;
bool vibro_set_modified = false;
bool need_quit = false;

#define SetId(x) x->setId(#x)
#define ARRAY_SIZE(x, y) sizeof(x) / sizeof(y)
#define ARRAY_AND_SIZE(x, y) x, ARRAY_SIZE(x, y)
#define HoldWidgets(x) widgetsHolder.Hold(ARRAY_AND_SIZE(x, gcn::Widget *))
#define HoldListModels(x) listModelsHolder.Hold(ARRAY_AND_SIZE(x, gcn::ListModel *))

// Constants
#define JS_DEV "/dev/input/js"
#define ACCEL_TOUCHBOOK "/dev/input/accel0"

template<class X>
class ResourceHolder
{
private:
    std::vector<X *> resources;
    
public:
    ~ResourceHolder()
    {
        Release();
    }

    void Hold(X **xs, int count)
    {
        for (int i = 0; i < count; i++)
            resources.push_back(xs[i]);
    }

    void Release()
    {
        for (uint i = 0; i < resources.size(); i++)
            delete resources[i];
        resources.clear();
    }
};

// Resource holders
ResourceHolder<gcn::Widget> widgetsHolder;
ResourceHolder<gcn::ListModel> listModelsHolder;

static int FillContainer(gcn::Container *cont, gcn::Widget **widgets, int count, int width, int startY = 0)
{
    int y = startY;
    for (int i = 0; i < count; i++)
    {
        gcn::Widget *widget = widgets[i];
        if (width > 0)
            widget->setWidth(width);
        cont->add(widget, 0, y);
        y += widget->getHeight();
    }
    return y;
}

static bool IsEqual(int x, int y)
{
    return (x == y);
}

static bool IsEqual(float x, float y)
{
    return (fabs(x - y) < FLT_EPSILON);
}

static bool IsEqual(std::string x, std::string y)
{
    return (x == y);
}

template<class X>
class GenericListModel: public gcn::ListModel
{
public:
    struct Element
    {
        std::string text;
        X value;

        Element(const std::string &_text, const X _value) :
            text(_text), value(_value)
        {}
    };

    typedef std::vector<Element> ElementsVector;

private:
    ElementsVector elements;

public:
    GenericListModel(const ElementsVector &_elements) :
        elements(_elements)
    {
    }

    std::string getElementAt(int i)
    {
        return (i >=0 && i < (int)elements.size() ? elements[i].text : "");
    }

    int getNumberOfElements()
    {
        return elements.size();
    }

    X getValueAt(int i)
    {
        const X &defaultVal = elements[0].value; // TODO: check for empty vector
        return (i >=0 && i < (int)elements.size() ? elements[i].value : defaultVal);
    }

    int findValueId(const X &val)
    {
        for (int i = 0; i < (int)elements.size(); i++)
        {
            if (IsEqual(elements[i].value, val))
                return i;
        }
        return -1;
    }
};

static GenericListModel<int> *CreateGenericListModel(const int *values, uint count)
{
    GenericListModel<int>::ElementsVector elements;
    for (uint i = 0; i < count; i++)
    {
        char buf[32];
        int val = values[i];
        sprintf(buf, "%d", val);
        elements.push_back(GenericListModel<int>::Element(buf, val));
    }
    return (new GenericListModel<int>(elements));
}

static GenericListModel<float> *CreateGenericListModel(const float *values, uint count)
{
    GenericListModel<float>::ElementsVector elements;
    for (uint i = 0; i < count; i++)
    {
        char buf[32];
        float val = values[i];
        sprintf(buf, "%.2f", val);
        elements.push_back(GenericListModel<float>::Element(buf, val));
    }
    return (new GenericListModel<float>(elements));
}

static GenericListModel<std::string> *CreateGenericListModel(const char **values, uint count)
{
    GenericListModel<std::string>::ElementsVector elements;
    for (uint i = 0; i < count; i++)
    {
        const char *val = values[i];
        elements.push_back(GenericListModel<std::string>::Element(val, val));
    }
    return (new GenericListModel<std::string>(elements));
}

static GenericListModel<int> *CreateGenericListModel(const char **names, const int *values, uint count)
{
    GenericListModel<int>::ElementsVector elements;
    for (uint i = 0; i < count; i++)
        elements.push_back(GenericListModel<int>::Element(names[i], values[i]));
    return (new GenericListModel<int>(elements));
}

class SuperDropDown : public gcn::DropDown
{
private:
    int scrollAreaHeight;

public:
    SuperDropDown(gcn::ListModel *listModel, gcn::ScrollArea *scrollArea, gcn::ListBox *listBox, int _scrollAreaHeight) :
        gcn::DropDown(listModel, scrollArea, listBox),
        scrollAreaHeight(_scrollAreaHeight)
    {
    }

    template<class X>
    X getSelectedValue()
    {
        int value_id = getSelected();
        return static_cast<GenericListModel<X> *>(getListModel())->getValueAt(value_id);
    }

    template<class X>
    void setSelectedValue(const X &val)
    {
        int value_id = static_cast<GenericListModel<X> *>(getListModel())->findValueId(val);
        if (value_id >= 0)
            setSelected(value_id);
    }

#ifdef FIX_DROPDOWN_HEIGHT
    void adjustHeight()
    {
        mListBox->setHeight(scrollAreaHeight);
        gcn::DropDown::adjustHeight();
    }

protected:
    void dropDown()
    {
        if (!mDroppedDown)
        {
            mDroppedDown = true;
            mFoldedUpHeight = getHeight();
            adjustHeight();

            if (getParent())
            {
                getParent()->moveToTop(this);
            }
        }

        mListBox->requestFocus();
    }

    void foldUp()
    {
        if (mDroppedDown)
        {
            mDroppedDown = false;
            adjustHeight();
            mInternalFocusHandler.focusNone();
        }
    }
#endif
};

static SuperDropDown *CreateDropDown(gcn::ListModel *listModel, int scrollBarWidth, int scrollAreaH)
{
    gcn::ScrollArea *scrollArea = new gcn::ScrollArea();
    gcn::ListBox *listBox = new gcn::ListBox();
    SuperDropDown *dropDown = new SuperDropDown(listModel, scrollArea, listBox, scrollAreaH);
    scrollArea->setScrollbarWidth(scrollBarWidth);
    scrollArea->setScrollPolicy(gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_AUTO);

    gcn::Widget *subWidgets[] = {scrollArea, listBox};
    HoldWidgets(subWidgets);
    
    return dropDown;
}

// Settings window stuff
gcn::Container *winCont = NULL;
gcn::Container *presetsCont = NULL;
gcn::Container *videoCont = NULL;
gcn::Container *inputCont = NULL;
gcn::Container *vibroCont = NULL;
gcn::Container *aboutCont = NULL;
int scrollHeight = 0;
int downScrollAreaH = 0;

// Presets tab stuff
gcn::Button *presetDesktopButton = NULL;
gcn::Button *presetFreerunnerButton = NULL;
gcn::Button *presetTouchbookButton = NULL;
gcn::Button *presetPandoraButton = NULL;

// Video tab stuff
gcn::CheckBox *chbGeomMax = NULL;
gcn::CheckBox *chbScroll = NULL;
SuperDropDown *downGeomX = NULL;
SuperDropDown *downGeomY = NULL;
SuperDropDown *downBpp = NULL;
SuperDropDown *downFullscreen = NULL;
SuperDropDown *downDelay = NULL;

// Input tab stuff
SuperDropDown *downInputType = NULL;
gcn::CheckBox *chbInputSwapXy = NULL;
gcn::CheckBox *chbInputInvertX = NULL;
gcn::CheckBox *chbInputInvertY = NULL;
SuperDropDown *downInputSens = NULL;
SuperDropDown *downBallSpeed = NULL;
gcn::Label *lblJsFile = NULL;
SuperDropDown *downJsFile = NULL;
gcn::Label *lblJsMax = NULL;
SuperDropDown *downJsMax = NULL;
gcn::Label *lblJsDelay = NULL;
SuperDropDown *downJsDelay = NULL;
gcn::Label *lblJsSdlNumber = NULL;
SuperDropDown *downJsSdlNumber = NULL;
gcn::Label *lblJsSdlMax = NULL;
SuperDropDown *downJsSdlMax = NULL;
gcn::Label *lblAccelFile = NULL;
SuperDropDown *downAccelFile = NULL;
gcn::Label *lblAccelMax = NULL;
SuperDropDown *downAccelMax = NULL;
gcn::Label *lblAccelDelay = NULL;
SuperDropDown *downAccelDelay = NULL;
int inputTabBaseHeight = 0;

// Vibro tab stuff
SuperDropDown *downVibroType = NULL;
SuperDropDown *downBumpMin = NULL;
SuperDropDown *downBumpMax = NULL;
gcn::Label *lblFrInterval = NULL;
SuperDropDown *downFrInterval = NULL;
int vibroTabBaseHeight = 0;

// Message box stuff
MsgBox *msgBox = NULL;

static void HideMsg()
{
    if (msgBox)
    {
        delete msgBox;
        msgBox = 0;
    }
}

static void ShowMsg(const std::string &text)
{
    HideMsg();
    msgBox = new MsgBox(winCont, top, text);
}

void TuneGeomMaxView()
{
    bool en = !chbGeomMax->isSelected();
    int tone = (en ? 255 : 150);
    gcn::Color bg(tone, tone, tone);
    downGeomX->setEnabled(en);
    downGeomX->setBackgroundColor(bg);
    downGeomY->setEnabled(en);
    downGeomY->setBackgroundColor(bg);
}

void TuneInputTypeView()
{
    InputType itype = (InputType)downInputType->getSelectedValue<int>();
    bool jsVis = false;
    bool jsSdlVis = false;
    bool accelVis = false;
    gcn::Widget *lastWidget = NULL;

    if (itype == INPUT_DUMMY)
    {
    }
    else if (itype == INPUT_KEYBOARD)
    {
    }
    else if (itype == INPUT_JOYSTICK)
    {
        jsVis = true;
        lastWidget = downJsDelay;
    }
    else if (itype == INPUT_JOYSTICK_SDL)
    {
        jsSdlVis = true;
        lastWidget = downJsSdlMax;
    }
    else if (itype == INPUT_ACCEL)
    {
        accelVis = true;
        lastWidget = downAccelDelay;
    }

    lblJsFile->setVisible(jsVis);
    downJsFile->setVisible(jsVis);
    lblJsMax->setVisible(jsVis);
    downJsMax->setVisible(jsVis);
    lblJsDelay->setVisible(jsVis);
    downJsDelay->setVisible(jsVis);
    lblJsSdlNumber->setVisible(jsSdlVis);
    downJsSdlNumber->setVisible(jsSdlVis);
    lblJsSdlMax->setVisible(jsSdlVis);
    downJsSdlMax->setVisible(jsSdlVis);
    lblAccelFile->setVisible(accelVis);
    downAccelFile->setVisible(accelVis);
    lblAccelMax->setVisible(accelVis);
    downAccelMax->setVisible(accelVis);
    lblAccelDelay->setVisible(accelVis);
    downAccelDelay->setVisible(accelVis);

    int tabHeight = (lastWidget ? lastWidget->getY() + lastWidget->getHeight() : inputTabBaseHeight);
    tabHeight = max(tabHeight + downScrollAreaH, scrollHeight);
    inputCont->setHeight(tabHeight);
}

void TuneVibroTypeView()
{
    VibroType vtype = (VibroType)downVibroType->getSelectedValue<int>();
    bool frVis = false;
    gcn::Widget *lastWidget = NULL;

    if (vtype == VIBRO_DUMMY)
    {
    }
    else if (vtype == VIBRO_FREERUNNER)
    {
        frVis = true;
        lastWidget = downFrInterval;
    }

    lblFrInterval->setVisible(frVis);
    downFrInterval->setVisible(frVis);

    int tabHeight = (lastWidget ? lastWidget->getY() + lastWidget->getHeight() : vibroTabBaseHeight);
    tabHeight = max(tabHeight + downScrollAreaH, scrollHeight);
    vibroCont->setHeight(tabHeight);
}

void TuneView()
{
    TuneGeomMaxView();
    TuneInputTypeView();
    TuneVibroTypeView();
}

static void LoadUiState()
{
    chbScroll->setSelected(user_set_new->scrolling);
    chbGeomMax->setSelected(user_set_new->geom_x == 0 && user_set_new->geom_y == 0);
    downGeomX->setSelectedValue<int>(user_set_new->geom_x);
    downGeomY->setSelectedValue<int>(user_set_new->geom_y);
    downBpp->setSelectedValue<int>(user_set_new->bpp);
    downFullscreen->setSelectedValue<int>(user_set_new->fullscreen_mode);
    downDelay->setSelectedValue<int>(user_set_new->frame_delay);

    downInputType->setSelectedValue<int>(user_set->input_type);
    chbInputSwapXy->setSelected(user_set->input_calibration_data.swap_xy);
    chbInputInvertX->setSelected(user_set->input_calibration_data.invert_x);
    chbInputInvertY->setSelected(user_set->input_calibration_data.invert_y);
    downInputSens->setSelectedValue<float>(user_set->input_calibration_data.sens);
    downBallSpeed->setSelectedValue<float>(user_set->ball_speed);
    downJsFile->setSelectedValue<std::string>(user_set->input_joystick_data.fname);
    downJsMax->setSelectedValue<int>(user_set->input_joystick_data.max_axis);
    downJsDelay->setSelectedValue<int>(user_set->input_joystick_data.interval);
    downJsSdlNumber->setSelectedValue<int>(user_set->input_joystick_sdl_data.number);
    downJsSdlMax->setSelectedValue<int>(user_set->input_joystick_sdl_data.max_axis);
    downAccelFile->setSelectedValue<std::string>(user_set->input_accel_data.fname);
    downAccelMax->setSelectedValue<int>(user_set->input_accel_data.max_axis);
    downAccelDelay->setSelectedValue<int>(user_set->input_accel_data.interval);

    downVibroType->setSelectedValue<int>(user_set->vibro_type);
    downBumpMin->setSelectedValue<float>(user_set->bump_min_speed);
    downBumpMax->setSelectedValue<float>(user_set->bump_max_speed);
    downFrInterval->setSelectedValue<int>(user_set->vibro_freeerunner_data.duration);

    TuneView();
}

static void SaveUiState()
{
    // Check for modifications
    bool geom_max = (user_set_new->geom_x == 0 && user_set_new->geom_x == 0);
    bool geom_xy_modified =
        (user_set_new->geom_x != downGeomX->getSelectedValue<int>()) ||
        (user_set_new->geom_y != downGeomY->getSelectedValue<int>());
    bool geom_modified =
        (geom_max != chbGeomMax->isSelected()) ||
        (!geom_max && geom_xy_modified);
    video_set_modified =
        (user_set_new->scrolling != chbScroll->isSelected()) ||
        geom_modified ||
        (user_set_new->bpp != downBpp->getSelectedValue<int>()) ||
        (user_set_new->fullscreen_mode != (FullscreenMode)downFullscreen->getSelectedValue<int>()) ||
        (user_set_new->frame_delay != downDelay->getSelectedValue<int>());
    input_set_modified =
        (user_set->input_type != (InputType)downInputType->getSelectedValue<int>()) ||
        (user_set->input_calibration_data.swap_xy != chbInputSwapXy->isSelected()) ||
        (user_set->input_calibration_data.invert_x != chbInputInvertX->isSelected()) ||
        (user_set->input_calibration_data.invert_y != chbInputInvertY->isSelected()) ||
        (user_set->input_calibration_data.sens != downInputSens->getSelectedValue<float>()) ||
        //(user_set->ball_speed != downBallSpeed->getSelectedValue<float>()) ||
        (user_set->input_joystick_data.fname != downJsFile->getSelectedValue<std::string>()) ||
        (user_set->input_joystick_data.max_axis != downJsMax->getSelectedValue<int>()) ||
        (user_set->input_joystick_data.interval != downJsDelay->getSelectedValue<int>()) ||
        (user_set->input_joystick_sdl_data.number != downJsSdlNumber->getSelectedValue<int>()) ||
        (user_set->input_joystick_sdl_data.max_axis != downJsSdlMax->getSelectedValue<int>()) ||
        (user_set->input_accel_data.fname != downAccelFile->getSelectedValue<std::string>()) ||
        (user_set->input_accel_data.max_axis != downAccelMax->getSelectedValue<int>()) ||
        (user_set->input_accel_data.interval != downAccelDelay->getSelectedValue<int>());
    vibro_set_modified =
        (user_set->vibro_type != (VibroType)downVibroType->getSelectedValue<int>()) ||
        //(user_set->bump_min_speed != downBumpMin->getSelectedValue<float>()) ||
        //(user_set->bump_max_speed != downBumpMax->getSelectedValue<float>()) ||
        (user_set->vibro_freeerunner_data.duration != downFrInterval->getSelectedValue<int>());

    // Set new values
    user_set_new->scrolling = chbScroll->isSelected();
    if (chbGeomMax->isSelected())
    {
        user_set_new->geom_x = 0;
        user_set_new->geom_y = 0;
    }
    else
    {
        user_set_new->geom_x = downGeomX->getSelectedValue<int>();
        user_set_new->geom_y = downGeomY->getSelectedValue<int>();
    }
    user_set_new->bpp = downBpp->getSelectedValue<int>();
    user_set_new->fullscreen_mode = (FullscreenMode)downFullscreen->getSelectedValue<int>();
    user_set_new->frame_delay = downDelay->getSelectedValue<int>();

    user_set->input_type = (InputType)downInputType->getSelectedValue<int>();
    user_set->input_calibration_data.swap_xy = chbInputSwapXy->isSelected();
    user_set->input_calibration_data.invert_x = chbInputInvertX->isSelected();
    user_set->input_calibration_data.invert_y = chbInputInvertY->isSelected();
    user_set->input_calibration_data.sens = downInputSens->getSelectedValue<float>();
    user_set->ball_speed = downBallSpeed->getSelectedValue<float>();
    if (user_set->input_joystick_data.fname)
        free(user_set->input_joystick_data.fname);
    user_set->input_joystick_data.fname = strdup(downJsFile->getSelectedValue<std::string>().c_str());
    user_set->input_joystick_data.max_axis = downJsMax->getSelectedValue<int>();
    user_set->input_joystick_data.interval = downJsDelay->getSelectedValue<int>();
    user_set->input_joystick_sdl_data.number = downJsSdlNumber->getSelectedValue<int>();
    user_set->input_joystick_sdl_data.max_axis = downJsSdlMax->getSelectedValue<int>();
    if (user_set->input_accel_data.fname)
        free(user_set->input_accel_data.fname);
    user_set->input_accel_data.fname = strdup(downAccelFile->getSelectedValue<std::string>().c_str());
    user_set->input_accel_data.max_axis = downAccelMax->getSelectedValue<int>();
    user_set->input_accel_data.interval = downAccelDelay->getSelectedValue<int>();

    user_set->vibro_type = (VibroType)downVibroType->getSelectedValue<int>();
    user_set->bump_min_speed = downBumpMin->getSelectedValue<float>();
    user_set->bump_max_speed = downBumpMax->getSelectedValue<float>();
    user_set->vibro_freeerunner_data.duration = downFrInterval->getSelectedValue<int>();

    // Display message if necessary
    if (video_set_modified)
    {
        need_quit = true;
        ShowMsg("New video settings\nwill be applied\nat the next game start");
    }
    else
        running = false;
}

class SaveActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        SaveUiState();
    }
};

class GeomMaxActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        TuneGeomMaxView();
    }
};

class InputTypeActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        TuneInputTypeView();
    }
};

class VibroTypeActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        TuneVibroTypeView();
    }
};

static void RestoreUiDefaults()
{
    chbScroll->setSelected(false);
    chbGeomMax->setSelected(true);
    downGeomX->setSelectedValue<int>(0);
    downGeomY->setSelectedValue<int>(0);
    downBpp->setSelectedValue<int>(0);
    downFullscreen->setSelectedValue<int>(FULLSCREEN_NONE);
    downDelay->setSelectedValue<int>(2);

    downInputType->setSelectedValue<int>(INPUT_KEYBOARD);
    chbInputSwapXy->setSelected(false);
    chbInputInvertX->setSelected(false);
    chbInputInvertY->setSelected(false);
    downInputSens->setSelectedValue<float>(0.7);
    downBallSpeed->setSelectedValue<float>(1.0);
    downJsFile->setSelectedValue<std::string>(JS_DEV "0");
    downJsMax->setSelectedValue<int>(32768);
    downJsDelay->setSelectedValue<int>(2);
    downJsSdlNumber->setSelectedValue<int>(0);
    downJsSdlMax->setSelectedValue<int>(32768);
    downAccelFile->setSelectedValue<std::string>(ACCEL_TOUCHBOOK);
    downAccelMax->setSelectedValue<int>(64);
    downAccelDelay->setSelectedValue<int>(2);

    downVibroType->setSelectedValue<int>(VIBRO_DUMMY);
    downBumpMin->setSelectedValue<float>(1.5);
    downBumpMax->setSelectedValue<float>(15.0);
    downFrInterval->setSelectedValue<int>(22);
}

class PresetActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        std::string note;
        const std::string &sourceId = actionEvent.getSource()->getId();
        RestoreUiDefaults();
        if (sourceId == presetDesktopButton->getId())
        {
            note = "Use the arrow keys\nto control the ball.";
        }
        else if (sourceId == presetFreerunnerButton->getId())
        {
            downFullscreen->setSelectedValue<int>(FULLSCREEN_INGAME);

            downInputType->setSelectedValue<int>(INPUT_JOYSTICK);
            chbInputInvertY->setSelected(true);
            downInputSens->setSelectedValue<float>(1.0);
            downBallSpeed->setSelectedValue<float>(1.4);
            downJsFile->setSelectedValue<std::string>(JS_DEV "1");
            downJsMax->setSelectedValue<int>(1000);

            downVibroType->setSelectedValue<int>(VIBRO_FREERUNNER);
            downBumpMin->setSelectedValue<float>(3.0);
            downBumpMax->setSelectedValue<float>(40.0);
            
            note = "Tilt the smartphone\nto control the ball.";
        }
        else if (sourceId == presetTouchbookButton->getId())
        {
            chbScroll->setSelected(true);
            downFullscreen->setSelectedValue<int>(FULLSCREEN_INGAME);

            downInputType->setSelectedValue<int>(INPUT_ACCEL);
            chbInputSwapXy->setSelected(true);
            downInputSens->setSelectedValue<float>(1.0);
            downBallSpeed->setSelectedValue<float>(1.8);

            note = "Tilt the tablet\nto control the ball.";
        }
        else if (sourceId == presetPandoraButton->getId())
        {
            chbScroll->setSelected(true);
            downFullscreen->setSelectedValue<int>(FULLSCREEN_INGAME);

            downInputType->setSelectedValue<int>(INPUT_JOYSTICK);
            downJsFile->setSelectedValue<std::string>(JS_DEV "1");

            note = "Use the left nub\n(in joystick mode)\nto control the ball.";
        }
        TuneView();
        std::string text = "Preset applied";
        if (!note.empty())
            text += ".\n" + note;
        ShowMsg(text);
    }
};

class QuitActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        running = false;
    }
};

class QuitMouseListener : public gcn::MouseListener
{
    void mouseClicked(gcn::MouseEvent &mouseEvent)
    {
        if (mouseEvent.getSource()->getId() == top->getId())
        {
            if (msgBox)
                msgBox->SetClicked(true);
            else
                running = false;
        }
    }
};

class CalPerformActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        calibration_requested = true;
        ShowMsg("Calibration will start\nwhen you return\nto the game");
    }
};

class CalResetActionListener : public gcn::ActionListener
{
    void action(const gcn::ActionEvent &actionEvent)
    {
        calibration_requested = false;
        user_set->input_calibration_data.cal_x = 0;
        user_set->input_calibration_data.cal_y = 0;
        ShowMsg("Calibration data\nwas cleared");
    }
};

// Action listeners
SaveActionListener saveActionListener;
PresetActionListener presetActionListener;
QuitActionListener quitActionListener;
QuitMouseListener quitMouseListener;
CalPerformActionListener calPerformActionListener;
CalResetActionListener calResetActionListener;
GeomMaxActionListener geomMaxActionListener;
InputTypeActionListener inputTypeActionListener;
VibroTypeActionListener vibroTypeActionListener;

static void halt()
{
    // Destroy custom stuff
    HideMsg();
    widgetsHolder.Release();
    listModelsHolder.Release();

    // Destroy Guichan stuff
    delete infoFont;
    delete titleFont;
    delete font;
    delete top;
    delete gui;

    // Destroy Guichan SDL stuff
    delete input;
    delete graphics;
    delete imageLoader;
}

void checkInput()
{
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
        } else if (event.type == SDL_QUIT)
        {
            running = false;
        }

        /*
         * Now that we are done polling and using SDL events we pass
         * the leftovers to the SDLInput object to later be handled by
         * the Gui.
         */
        input->pushInput(event);
    }

    if (msgBox && msgBox->IsClicked())
    {
        delete msgBox;
        msgBox = 0;
        if (need_quit)
        {
            running = false;
            need_quit = false;
        }
    }
}

void run()
{
    while (running)
    {
        // Poll input
        checkInput();
        // Let the gui perform it's logic (like handle input)
        gui->logic();
        // Draw the gui
        gui->draw();
        // Update the screen
        SDL_Flip(_screen);
        // Sleep
        SDL_Delay(user_set->frame_delay);
    }
}

void settings_init(SDL_Surface *disp, int font_height, User *_user_set, User *_user_set_new)
{
    _screen = disp;
    user_set = _user_set;
    user_set_new = _user_set_new;

    const int SETTINGS_WIDTH_FACTOR = 20;
    gcn::Rectangle guiRect(0, 0, disp->w, disp->h);
    gcn::Rectangle winRect;
    winRect.width = min(font_height * SETTINGS_WIDTH_FACTOR, guiRect.width);
    winRect.height = min(winRect.width * 3 / 4, guiRect.height);
    font_height = max(winRect.width / SETTINGS_WIDTH_FACTOR, 1);
    winRect.x = (guiRect.width - winRect.width) / 2;
    winRect.y = (guiRect.height - winRect.height) / 3;

    /*
     * Now it's time for Guichan SDL stuff
     */
    imageLoader = new gcn::SDLImageLoader();
    // The ImageLoader in use is static and must be set to be
    // able to load images
    gcn::Image::setImageLoader(imageLoader);
    graphics = new gcn::SDLGraphics();
    // Set the target for the graphics object to be the screen.
    // In other words, we will draw to the screen.
    // Note, any surface will do, it doesn't have to be the screen.
    graphics->setTarget(_screen);
    input = new gcn::SDLInput();

    /*
     * Last but not least it's time to initialize and create the gui
     * with Guichan stuff.
     */
    top = new gcn::Container();
    // Set the dimension of the top container to match the screen.
    top->setDimension(guiRect);
    top->setOpaque(false);
    gui = new SuperGui();
    // Set gui to use the SDLGraphics object.
    gui->setGraphics(graphics);
    // Set gui to use the SDLInput object
    gui->setInput(input);
    // Set the top container
    gui->setTop(top);

    // Load the font
    int font_color[4] = {0};
    font = new Font(DEFAULT_FONT_FILE, font_height, font_color);
    titleFont = new Font(DEFAULT_FONT_FILE, font_height * 3 / 2, font_color, true);
    infoFont = new Font(DEFAULT_FONT_FILE, font_height * 2 / 3, font_color);

    // The global font is static and must be set.
    gcn::Widget::setGlobalFont(font);

    /*
     * Init settings window
     */
    winCont = new gcn::Container();
    gcn::Button *okButton = new gcn::Button("OK");
    gcn::Button *cancelButton = new gcn::Button("Cancel");
    okButton->setWidth(winRect.width / 2);
    cancelButton->setWidth(winRect.width / 2);
    winCont->setSize(winRect.width, winRect.height);
    okButton->addActionListener(&saveActionListener);
    cancelButton->addActionListener(&quitActionListener);
    int tabHeight = winRect.height - okButton->getHeight();
    winCont->add(cancelButton, winRect.width - cancelButton->getWidth(), tabHeight);
    winCont->add(okButton, cancelButton->getX() - okButton->getWidth(), tabHeight);

    SuperTabbedArea *superTabbedArea = new SuperTabbedArea();
    presetsCont = new gcn::Container();
    videoCont = new gcn::Container();
    inputCont = new gcn::Container();
    vibroCont = new gcn::Container();
    aboutCont = new gcn::Container();

    gcn::ScrollArea *presetsScroll = new gcn::ScrollArea();
    gcn::ScrollArea *videoScroll = new gcn::ScrollArea();
    gcn::ScrollArea *inputScroll = new gcn::ScrollArea();
    gcn::ScrollArea *vibroScroll = new gcn::ScrollArea();
    gcn::ScrollArea *aboutScroll = new gcn::ScrollArea();

    scrollHeight = tabHeight - font_height * 1.7;
    downScrollAreaH = scrollHeight / 2;
    superTabbedArea->setSize(winRect.width, tabHeight);
    presetsScroll->setSize(winRect.width, scrollHeight);
    videoScroll->setSize(winRect.width, scrollHeight);
    inputScroll->setSize(winRect.width, scrollHeight);
    vibroScroll->setSize(winRect.width, scrollHeight);
    aboutScroll->setSize(winRect.width, scrollHeight);

    presetsScroll->setScrollPolicy(gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_AUTO);
    videoScroll->setScrollPolicy(gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_AUTO);
    inputScroll->setScrollPolicy(gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_AUTO);
    vibroScroll->setScrollPolicy(gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_AUTO);
    aboutScroll->setScrollPolicy(gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_AUTO);

    int scrollBarW = winRect.width / 20;
    int scrolledTabW = winRect.width - scrollBarW;
    presetsScroll->setScrollbarWidth(scrollBarW);
    videoScroll->setScrollbarWidth(scrollBarW);
    inputScroll->setScrollbarWidth(scrollBarW);
    vibroScroll->setScrollbarWidth(scrollBarW);
    aboutScroll->setScrollbarWidth(scrollBarW);

    presetsScroll->setContent(presetsCont);
    videoScroll->setContent(videoCont);
    inputScroll->setContent(inputCont);
    vibroScroll->setContent(vibroCont);
    aboutScroll->setContent(aboutCont);

    superTabbedArea->addTab("Presets", presetsScroll);
    superTabbedArea->addTab("Video", videoScroll);
    superTabbedArea->addTab("Input", inputScroll);
    superTabbedArea->addTab("Vibro", vibroScroll);
    superTabbedArea->addTab("About", aboutScroll);

    winCont->add(superTabbedArea);
    top->add(winCont, winRect.x, winRect.y);
    top->addMouseListener(&quitMouseListener);
    SetId(top);

    gcn::Widget *setWinWidgets[] = {winCont, superTabbedArea, presetsCont,
        videoCont, inputCont, vibroCont, aboutCont, presetsScroll, videoScroll,
        inputScroll, vibroScroll, aboutScroll, okButton, cancelButton};
    HoldWidgets(setWinWidgets);

    /*
     * Init presets tab
     */
    presetDesktopButton = new gcn::Button("Desktop/laptop");
    presetFreerunnerButton = new gcn::Button("Openmoko FreeRunner");
    presetTouchbookButton = new gcn::Button("AI Touch Book");
    presetPandoraButton = new gcn::Button("OpenPandora");

    SetId(presetDesktopButton);
    SetId(presetFreerunnerButton);
    SetId(presetTouchbookButton);
    SetId(presetPandoraButton);

    presetDesktopButton->addActionListener(&presetActionListener);
    presetFreerunnerButton->addActionListener(&presetActionListener);
    presetTouchbookButton->addActionListener(&presetActionListener);
    presetPandoraButton->addActionListener(&presetActionListener);

    gcn::Widget *presetsWidgets[] = {presetDesktopButton, presetFreerunnerButton,
        presetTouchbookButton, presetPandoraButton};
    int presetsTabHeight = FillContainer(presetsCont, ARRAY_AND_SIZE(presetsWidgets, gcn::Widget *), winRect.width);
    presetsCont->setSize(winRect.width, max(presetsTabHeight, scrollHeight));
    HoldWidgets(presetsWidgets);

    /*
     * Init video tab
     */
    chbGeomMax = new gcn::CheckBox("Maximum window size");
    gcn::Label *lblGeomX = new gcn::Label("Window width (px)");
    gcn::Label *lblGeomY = new gcn::Label("Window height (px)");
    gcn::Label *lblBpp = new gcn::Label("Color depth");
    chbScroll = new gcn::CheckBox("Display scrolling");
    gcn::Label *lblFullscreen = new gcn::Label("Fullscreen mode");
    gcn::Label *lblDelay = new gcn::Label("Frame rendering delay (ms)");

    const int geomVariants[] = {240, 320, 480, 600, 640, 720, 768, 800, 900,
        1024, 1050, 1080, 1200, 1280, 1360, 1366, 1440, 1680, 1920};
    gcn::ListModel *geomListModel = CreateGenericListModel(ARRAY_AND_SIZE(geomVariants, int));

    const int bppVariants[] = {0, 16, 32};
    const char *bppVariantNames[] = {"auto", "16 bpp", "32 bpp"};
    gcn::ListModel *bppListModel = CreateGenericListModel(bppVariantNames, ARRAY_AND_SIZE(bppVariants, int));

    const int fsVariants[] = {FULLSCREEN_NONE, FULLSCREEN_INGAME, FULLSCREEN_ALWAYS};
    const char *fsVariantNames[] = {FULLSCREEN_NONE_STR, FULLSCREEN_INGAME_STR, FULLSCREEN_ALWAYS_STR};
    gcn::ListModel *fsListModel = CreateGenericListModel(fsVariantNames, ARRAY_AND_SIZE(fsVariants, int));

    const int delayVariants[] = {0, 1, 2, 5, 10, 20, 50};
    gcn::ListModel *delayListModel = CreateGenericListModel(ARRAY_AND_SIZE(delayVariants, int));

    gcn::ListModel *videoListModels[] = {geomListModel, bppListModel, fsListModel, delayListModel};
    HoldListModels(videoListModels);

    downGeomX = CreateDropDown(geomListModel, scrollBarW, downScrollAreaH);
    downGeomY = CreateDropDown(geomListModel, scrollBarW, downScrollAreaH);
    downBpp = CreateDropDown(bppListModel, scrollBarW, downScrollAreaH);
    downFullscreen = CreateDropDown(fsListModel, scrollBarW, downScrollAreaH);
    downDelay = CreateDropDown(delayListModel, scrollBarW, downScrollAreaH);

    chbGeomMax->addActionListener(&geomMaxActionListener);

    gcn::Widget *videoWidgets[] = {chbScroll, chbGeomMax, lblGeomX, downGeomX,
        lblGeomY, downGeomY, lblBpp, downBpp, lblFullscreen, downFullscreen,
        lblDelay, downDelay};
    int videoTabHeight = FillContainer(videoCont, ARRAY_AND_SIZE(videoWidgets, gcn::Widget *), scrolledTabW);
    videoCont->setSize(winRect.width, max(videoTabHeight + downScrollAreaH, scrollHeight));
    HoldWidgets(videoWidgets);

    /*
     * Init input tab
     */
    const int inputTypeVariants[] = {INPUT_DUMMY, INPUT_KEYBOARD, INPUT_JOYSTICK, INPUT_JOYSTICK_SDL, INPUT_ACCEL};
    const char *inputTypeVariantNames[] = {INPUT_DUMMY_STR, INPUT_KEYBOARD_STR, INPUT_JOYSTICK_STR, INPUT_JOYSTICK_SDL_STR, INPUT_ACCEL_STR};
    gcn::ListModel *inputTypeListModel = CreateGenericListModel(inputTypeVariantNames, ARRAY_AND_SIZE(inputTypeVariants, int));

    const float inputSensVariants[] = {
        0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
        1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0,
        2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0
    };
    gcn::ListModel *inputSensListModel = CreateGenericListModel(ARRAY_AND_SIZE(inputSensVariants, float));

    const char *jsFileVariants[] = {JS_DEV "0", JS_DEV "1", JS_DEV "2", JS_DEV "3"};
    gcn::ListModel *jsFileListModel = CreateGenericListModel(ARRAY_AND_SIZE(jsFileVariants, char *));

    const int jsSdlNumberVariants[] = {0, 1, 2, 3, 4, 5, 6, 7};
    gcn::ListModel *jsSdlNumberListModel = CreateGenericListModel(ARRAY_AND_SIZE(jsSdlNumberVariants, int));

    const char *accelFileVariants[] = {ACCEL_TOUCHBOOK};
    gcn::ListModel *accelFileListModel = CreateGenericListModel(ARRAY_AND_SIZE(accelFileVariants, char *));

    const int axisMaxVariants[] = {32, 64, 100, 128, 256, 512, 1000, 1024, 8192, 16384, 32768};
    gcn::ListModel *axisMaxListModel = CreateGenericListModel(ARRAY_AND_SIZE(axisMaxVariants, int));

    gcn::ListModel *inputListModels[] = {inputTypeListModel, inputSensListModel, jsFileListModel, jsSdlNumberListModel, accelFileListModel, axisMaxListModel};
    HoldListModels(inputListModels);

    gcn::Label *lblInputType = new gcn::Label("Input device type");
    downInputType = CreateDropDown(inputTypeListModel, scrollBarW, downScrollAreaH);
    gcn::Button *btnInputCal = new gcn::Button("Calibrate");
    gcn::Button *btnInputCalReset = new gcn::Button("Reset calibration");
    chbInputSwapXy = new gcn::CheckBox("Swap X and Y axes");
    chbInputInvertX = new gcn::CheckBox("Invert X axis");
    chbInputInvertY = new gcn::CheckBox("Invert Y axis");

    gcn::Label *lblInputSens = new gcn::Label("Sensitivity");
    downInputSens = CreateDropDown(inputSensListModel, scrollBarW, downScrollAreaH);
    gcn::Label *lblBallSpeed = new gcn::Label("Ball speed");
    downBallSpeed = CreateDropDown(inputSensListModel, scrollBarW, downScrollAreaH);

    lblJsFile = new gcn::Label("Joystick file");
    downJsFile = CreateDropDown(jsFileListModel, scrollBarW, downScrollAreaH);
    lblJsMax = new gcn::Label("Max joystick axis offset");
    downJsMax = CreateDropDown(axisMaxListModel, scrollBarW, downScrollAreaH);
    lblJsDelay = new gcn::Label("Joystick reading interval (ms)");
    downJsDelay = CreateDropDown(delayListModel, scrollBarW, downScrollAreaH);

    lblJsSdlNumber = new gcn::Label("Joystick number");
    downJsSdlNumber = CreateDropDown(jsSdlNumberListModel, scrollBarW, downScrollAreaH);
    lblJsSdlMax = new gcn::Label("Max joystick axis offset");
    downJsSdlMax = CreateDropDown(axisMaxListModel, scrollBarW, downScrollAreaH);

    lblAccelFile = new gcn::Label("Accelerometer file");
    downAccelFile = CreateDropDown(accelFileListModel, scrollBarW, downScrollAreaH);
    lblAccelMax = new gcn::Label("Max accelerometer axis offset");
    downAccelMax = CreateDropDown(axisMaxListModel, scrollBarW, downScrollAreaH);
    lblAccelDelay = new gcn::Label("Accel. reading interval (ms)");
    downAccelDelay = CreateDropDown(delayListModel, scrollBarW, downScrollAreaH);

    downInputType->addActionListener(&inputTypeActionListener);
    btnInputCal->addActionListener(&calPerformActionListener);
    btnInputCalReset->addActionListener(&calResetActionListener);

    gcn::Widget *inputBaseWidgets[] = {lblInputType, downInputType, btnInputCal, btnInputCalReset,
        chbInputSwapXy, chbInputInvertX, chbInputInvertY, lblInputSens, downInputSens,
        lblBallSpeed, downBallSpeed};
    gcn::Widget *inputJsWidgets[] = {lblJsFile, downJsFile, lblJsMax, downJsMax, lblJsDelay, downJsDelay};
    gcn::Widget *inputJsSdlWidgets[] = {lblJsSdlNumber, downJsSdlNumber, lblJsSdlMax, downJsSdlMax};
    gcn::Widget *inputAccelWidgets[] = {lblAccelFile, downAccelFile, lblAccelMax, downAccelMax, lblAccelDelay, downAccelDelay};
    inputTabBaseHeight = FillContainer(inputCont, ARRAY_AND_SIZE(inputBaseWidgets, gcn::Widget *), scrolledTabW);
    FillContainer(inputCont, ARRAY_AND_SIZE(inputJsWidgets, gcn::Widget *), scrolledTabW, inputTabBaseHeight);
    FillContainer(inputCont, ARRAY_AND_SIZE(inputJsSdlWidgets, gcn::Widget *), scrolledTabW, inputTabBaseHeight);
    FillContainer(inputCont, ARRAY_AND_SIZE(inputAccelWidgets, gcn::Widget *), scrolledTabW, inputTabBaseHeight);
    inputCont->setSize(winRect.width, max(inputTabBaseHeight + downScrollAreaH, scrollHeight));
    HoldWidgets(inputBaseWidgets);
    HoldWidgets(inputJsWidgets);
    HoldWidgets(inputJsSdlWidgets);
    HoldWidgets(inputAccelWidgets);

    /*
     * Init vibro tab
     */
    const int vibroTypeVariants[] = {VIBRO_DUMMY, VIBRO_FREERUNNER};
    const char *vibroTypeVariantNames[] = {VIBRO_DUMMY_STR, VIBRO_FREERUNNER_STR};
    gcn::ListModel *vibroTypeListModel = CreateGenericListModel(vibroTypeVariantNames, ARRAY_AND_SIZE(vibroTypeVariants, int));

    const float bumpMinVariants[] = {0.7, 1.0, 1.2, 1.5, 1.7, 2.0, 2.2, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
    gcn::ListModel *bumpMinListModel = CreateGenericListModel(ARRAY_AND_SIZE(bumpMinVariants, float));

    const float bumpMaxVariants[] = {10.0, 12.0, 15.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0, 120.0, 150.0};
    gcn::ListModel *bumpMaxListModel = CreateGenericListModel(ARRAY_AND_SIZE(bumpMaxVariants, float));

    const int frIntervalVariants[] = {10, 12, 15, 17, 20, 22, 25, 27, 30, 33, 35, 37, 40, 42, 45, 47, 50};
    gcn::ListModel *frIntervalListModel = CreateGenericListModel(ARRAY_AND_SIZE(frIntervalVariants, int));

    gcn::ListModel *vibroListModels[] = {vibroTypeListModel, bumpMinListModel, bumpMaxListModel, frIntervalListModel};
    HoldListModels(vibroListModels);

    gcn::Label *lblVibroType = new gcn::Label("Vibro device type");
    downVibroType = CreateDropDown(vibroTypeListModel, scrollBarW, downScrollAreaH);
    gcn::Label *lblBumpMin = new gcn::Label("Min bump speed");
    downBumpMin = CreateDropDown(bumpMinListModel, scrollBarW, downScrollAreaH);
    gcn::Label *lblBumpMax = new gcn::Label("Max bump speed");
    downBumpMax = CreateDropDown(bumpMaxListModel, scrollBarW, downScrollAreaH);

    lblFrInterval = new gcn::Label("Vibration interval (ms)");
    downFrInterval = CreateDropDown(frIntervalListModel, scrollBarW, downScrollAreaH);
    
    downVibroType->addActionListener(&vibroTypeActionListener);

    gcn::Widget *vibroBaseWidgets[] = {lblVibroType, downVibroType,
        lblBumpMin, downBumpMin, lblBumpMax, downBumpMax};
    gcn::Widget *vibroFrWidgets[] = {lblFrInterval, downFrInterval};
    vibroTabBaseHeight = FillContainer(vibroCont, ARRAY_AND_SIZE(vibroBaseWidgets, gcn::Widget *), scrolledTabW);
    FillContainer(vibroCont, ARRAY_AND_SIZE(vibroFrWidgets, gcn::Widget *), scrolledTabW, vibroTabBaseHeight);
    vibroCont->setSize(winRect.width, max(vibroTabBaseHeight + downScrollAreaH, scrollHeight));
    HoldWidgets(vibroBaseWidgets);
    HoldWidgets(vibroFrWidgets);

    /*
     * Init about tab
     */
    gcn::Label *lblAbout1 = new gcn::Label("Mokomaze");
    gcn::Label *lblAbout2 = new gcn::Label("Ball-in-a-labyrinth game");
    gcn::Label *lblAbout3 = new gcn::Label("Copyright (C) 2009-2012 Anton Olkhovik");
    gcn::Label *lblAbout4 = new gcn::Label("http://mokomaze.sourceforge.net/");

    lblAbout1->setAlignment(gcn::Graphics::CENTER);
    lblAbout2->setAlignment(gcn::Graphics::CENTER);
    lblAbout3->setAlignment(gcn::Graphics::CENTER);
    lblAbout4->setAlignment(gcn::Graphics::CENTER);

    lblAbout1->setFont(titleFont);
    lblAbout3->setFont(infoFont);
    lblAbout4->setFont(infoFont);

    int lblsHeight = lblAbout1->getHeight() + lblAbout2->getHeight() +
        lblAbout3->getHeight() + lblAbout4->getHeight();
    int lblsY = (scrollHeight - lblsHeight) / 3;

    gcn::Widget *aboutWidgets[] = {lblAbout1, lblAbout2, lblAbout3, lblAbout4};
    int aboutTabHeight = FillContainer(aboutCont, ARRAY_AND_SIZE(aboutWidgets, gcn::Widget *), winRect.width, lblsY);
    aboutCont->setSize(winRect.width, max(aboutTabHeight, scrollHeight));
    HoldWidgets(aboutWidgets);
}

void settings_show(bool *_calibration_requested, bool *_video_set_modified, bool *_input_set_modified, bool *_vibro_set_modified)
{
    /*
     * Load UI state
     */
    LoadUiState();
    
    /*
     * Show window
     */
    running = true;
    need_quit = false;
    calibration_requested = false;
    video_set_modified = false;
    input_set_modified = false;
    vibro_set_modified = false;

    try
    {
        run();
    }
    // Catch all Guichan exceptions
    catch (gcn::Exception e)
    {
        std::cerr << e.getMessage() << std::endl;
        return;
    }
    // Catch all Std exceptions
    catch (std::exception e)
    {
        std::cerr << "Std exception: " << e.what() << std::endl;
        return;
    }
    // Catch all Unknown exceptions
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
        return;
    }

    if (_calibration_requested)
        *_calibration_requested = calibration_requested;
    if (_video_set_modified)
        *_video_set_modified = video_set_modified;
    if (_input_set_modified)
        *_input_set_modified = input_set_modified;
    if (_vibro_set_modified)
        *_vibro_set_modified = vibro_set_modified;
}

void settings_shutdown()
{
    halt();
}
