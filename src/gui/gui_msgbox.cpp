/*  gui_msgbox.cpp
 *
 *  (c) 2011 Anton Olkhovik <ant007h@gmail.com>
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

#include "gui_msgbox.h"

static void Split(const std::string &str, const std::string &delimiters, std::vector<std::string> &tokens)
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters. Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

MsgBox::MsgBox(gcn::Widget *_parent, gcn::Container *_top, const std::string &text) :
    parent(_parent),
    top(_top),
    clicked(false)
{
    const gcn::Rectangle &parentRect = parent->getDimension();
    int width = parentRect.width * 3 / 4;
    gcn::Color color(0xd4, 0xd4, 0xd4);
    
    cont = new gcn::Container();
    cont->setWidth(width);
    cont->setBaseColor(color);

    std::vector<std::string> lines;
    Split(text, "\n", lines);
    int height = 0;
    for (int i = 0; i < (int)lines.size(); i++)
    {
        gcn::Label *label = new gcn::Label(lines[i]);
        cont->add(label, 0, height);
        height += label->getHeight();
        label->setAlignment(gcn::Graphics::CENTER);
        label->setWidth(cont->getWidth());
        labels.push_back(label);
    }
    btn = new gcn::Button("OK");
    btn->setWidth(cont->getWidth());
    btn->setBaseColor(color);
    btn->addActionListener(this);

    cont->add(btn, cont->getWidth() - btn->getWidth(), height);
    height += btn->getHeight();
    cont->setHeight(height);

    int posX = parentRect.x + (parentRect.width - width) / 2;
    int posY = parentRect.y + (parentRect.height - height) * 2 / 5;
    top->add(cont, posX, posY);

    parent->setVisible(false);
}

MsgBox::~MsgBox()
{
    top->remove(cont);
    cont->clear();
    delete btn;
    for (unsigned int i = 0; i < labels.size(); i++)
        delete labels[i];
    delete cont;
    parent->setVisible(true);
}

void MsgBox::action(const gcn::ActionEvent &actionEvent)
{
    SetClicked(true);
}

bool MsgBox::IsClicked() const
{
    return clicked;
}

void MsgBox::SetClicked(bool val)
{
    clicked = val;
}
