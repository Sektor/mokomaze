/*  gui_msgbox.h
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

#ifndef GUI_MSGBOX_H
#define GUI_MSGBOX_H

#include <guichan.hpp>

class MsgBox : public gcn::ActionListener
{
private:
    gcn::Widget *parent;
    gcn::Container *top;
    std::vector<gcn::Label *> labels;
    gcn::Container *cont;
    gcn::Button *btn;
    bool clicked;
    void action(const gcn::ActionEvent &actionEvent);
public:
    MsgBox(gcn::Widget *_parent, gcn::Container *_top, const std::string &text);
    ~MsgBox();
    bool IsClicked() const;
    void SetClicked(bool val);
};

#endif /* GUI_MSGBOX_H */
