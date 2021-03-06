/*
Copyright (C) 2019 Remon Sijrier

This file is part of Traverso

Traverso is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
*/

#include "TGainGroupCommand.h"

#include "Gain.h"
#include "TAudioProcessingNode.h"
#include "Mixer.h"
#include "Utils.h"

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"


TGainGroupCommand::TGainGroupCommand(ContextItem *context, const QVariantList &args)
    : TCommand (context)
    , m_primaryGain(nullptr)
    , m_contextItem(context)
    , m_primaryGainOnly(false)
{
    m_canvasCursorFollowsMouseCursor = false;

    QString des = "";
    TAudioProcessingNode* data = qobject_cast<TAudioProcessingNode*>(context);
    QString name;
    if (data) {
        name = data->get_name();
    }

    if (!args.empty()) {
        des = QString(context->metaObject()->className()) + ": Reset gain";
    } else {
        des = "Gain (" + QString(context->metaObject()->className()) + " " + name + ")";
    }

    setText(des);

}

TGainGroupCommand::~ TGainGroupCommand()
{
    PENTERDES;

    for(auto gain : m_gainCommands) {
        delete gain;
    }
}

void TGainGroupCommand::set_cursor_shape(int useX, int useY)
{
    Q_UNUSED(useX);
    Q_UNUSED(useY);

    cpointer().set_canvas_cursor_shape(":/cursorGain");
}


int TGainGroupCommand::begin_hold()
{
    m_origPos = cpointer().scene_pos();
    cpointer().set_canvas_cursor_text(coefficient_to_dbstring(Gain::get_gain_from_object(m_contextItem)));
    return 1;
}

int TGainGroupCommand::finish_hold()
{
    return 1;
}

void TGainGroupCommand::cancel_action()
{
    PENTER;
    for(auto gain : m_gainCommands) {
        gain->cancel_action();
    }
}

void TGainGroupCommand::process_collected_number(const QString &collected)
{
    if (collected.size() == 0) {
        cpointer().set_canvas_cursor_text(" dB");
        return;
    }

    bool ok;
    audio_sample_t dbFactor = audio_sample_t(collected.toDouble(&ok));
    if (!ok) {
        if (collected.contains(".") || collected.contains("-")) {
            QString s = collected;
            s.append(" dB");
            cpointer().set_canvas_cursor_text(s);
        }
        return;
    }

    int rightfromdot = 0;
    if (collected.contains(".")) {
        rightfromdot = collected.size() - collected.lastIndexOf(".") - 1;
    }

    float newGain = dB_to_scale_factor(dbFactor);

    // Update the vieport's hold cursor with the _actuall_ gain value!
    if(rightfromdot) {
        cpointer().set_canvas_cursor_text(QByteArray::number(double(dbFactor), 'f', rightfromdot).append(" dB"));
    } else {
        cpointer().set_canvas_cursor_text(QByteArray::number(double(dbFactor)).append(" dB"));
    }

    if (m_primaryGainOnly) {
        m_primaryGain->set_new_gain(newGain);
    } else {
        for(auto gain : m_gainCommands) {
            gain->set_new_gain_numerical_input(newGain);
        }
    }
}

int TGainGroupCommand::jog()
{
    qreal diff = m_origPos.y() - cpointer().scene_y();

    if (m_primaryGainOnly) {
        m_primaryGain->process_mouse_move(diff);
    } else {
        for(auto gain : m_gainCommands) {
            gain->process_mouse_move(diff);
        }
    }

    cpointer().set_canvas_cursor_pos(m_origPos);

    // Update the vieport's hold cursor!
    cpointer().set_canvas_cursor_text(coefficient_to_dbstring(Gain::get_gain_from_object(m_contextItem)));

    return 1;
}


int TGainGroupCommand::prepare_actions()
{
    if (m_gainCommands.isEmpty()) {
        return -1;
    }

    for(auto gain : m_gainCommands) {
        if (gain->prepare_actions() == -1) {
            printf("one of the commands in the group failed prepare_actions\n");
            return -1;
        }
    }

    return 1;
}

int TGainGroupCommand::do_action()
{
    if (m_primaryGainOnly) {
        m_primaryGain->do_action();
    } else {
        for(auto gain : m_gainCommands) {
            gain->do_action();
        }
    }

    return 1;
}

int TGainGroupCommand::undo_action()
{
    if (m_primaryGainOnly) {
        m_primaryGain->undo_action();
    } else {
        for(auto gain : m_gainCommands) {
            gain->undo_action();
        }
    }

    return 1;
}

void TGainGroupCommand::add_command(Gain *cmd) {
    Q_ASSERT(cmd);

    if (!m_primaryGain) {
        m_primaryGain = cmd;
    }

    if (m_gainCommands.contains(cmd)) {
        return;
    }

    m_gainCommands.append(cmd);
}


void TGainGroupCommand::increase_gain(  )
{
    if (m_primaryGainOnly) {
        m_primaryGain->increase_gain();
    } else {
        for(auto gain : m_gainCommands) {
            gain->increase_gain();
        }
    }

    // Update the vieport's hold cursor with the _actuall_ gain value!
    cpointer().set_canvas_cursor_text(coefficient_to_dbstring(Gain::get_gain_from_object(m_contextItem)));
}

void TGainGroupCommand::decrease_gain()
{
    if (m_primaryGainOnly) {
        m_primaryGain->decrease_gain();
    } else {
        for(auto gain : m_gainCommands) {
            gain->decrease_gain();
        }
    }

    // Update the vieport's hold cursor with the _actuall_ gain value!
    cpointer().set_canvas_cursor_text(coefficient_to_dbstring(Gain::get_gain_from_object(m_contextItem)));
}

void TGainGroupCommand::reset_gain()
{
    for (auto gain : m_gainCommands) {
        gain->set_new_gain(1.0);
    }

    // Update the vieport's hold cursor with the _actuall_ gain value!
    cpointer().set_canvas_cursor_text(coefficient_to_dbstring(1.0));
}

void TGainGroupCommand::toggle_primary_gain_only()
{
    m_primaryGainOnly = !m_primaryGainOnly;
}

// eof

