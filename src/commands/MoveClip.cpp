/*
Copyright (C) 2005-2008 Remon Sijrier 

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

#include "MoveClip.h"

#include "AudioClip.h"
#include "AudioClipManager.h"
#include "ContextPointer.h"
#include "TInputEventDispatcher.h"
#include "SnapList.h"
#include "Sheet.h"
#include "AudioTrack.h"
#include "TimeLine.h"

#include "ClipsViewPort.h"
#include "SheetView.h"
#include "AudioTrackView.h"
#include "AudioClipView.h"

#include "Zoom.h"


// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"

/**
  *	\class MoveClip
    \brief A Command class for Dragging or Copy-dragging an AudioClip

    \sa TraversoCommands
 */


/**
 *	Creates  a Move Clip or Copy Clip Command object.
 */
MoveClip::MoveClip(ViewItem* view, const QVariantList& args)
    : TMoveCommand(nullptr, view->get_context(), "")
    , m_actionType(UNDEFINED)
    , mcd(new MoveClipData)
{
    QString action = "No action supplied in args";

    if (!args.isEmpty()) {
        action = args.at(0).toString();
    }
    if (args.size() > 1) {
        mcd->verticalOnly = args.at(1).toBool();
    } else {
        mcd->verticalOnly = false;
    }

    QString des;
    if (action == "copy") {
        des = tr("Copy Clip");
        m_actionType = COPY;
    } else if (action == "move") {
        des = tr("Move Clip");
        m_actionType = MOVE;
    } else if (action == "move_to_start") {
        des = tr("Move Clip To Start");
        m_actionType = MOVE_TO_START;
    } else if (action == "move_to_end") {
        des = tr("Move Clip To End");
        m_actionType = MOVE_TO_END;
    } else if (action == "fold_sheet") {
        des = tr("Fold Sheet");
        m_actionType = FOLD_SHEET;
    } else if (action == "fold_track") {
        des = tr("Fold Track");
        m_actionType = FOLD_TRACK;
    } else if (action == "fold_markers") {
        des = tr("Fold Markers");
        m_actionType = FOLD_MARKERS;
    } else {
        PERROR(QString("MoveClip: Unknown action type: %1").arg(action));
        m_actionType = UNDEFINED;
    }

    setText(des);

    if (m_actionType == FOLD_SHEET || m_actionType == FOLD_TRACK || m_actionType == FOLD_MARKERS) {

        QList<AudioClip*> movingClips;
        QList<AudioTrack*> tracks;

        if (m_actionType == FOLD_TRACK) {
            AudioTrackView* tv = qobject_cast<AudioTrackView*>(view);
            Q_ASSERT(tv);
            d->sv= tv->get_sheetview();
            tracks.append(tv->get_track());
        } else if (m_actionType == FOLD_SHEET) {
            d->sv = qobject_cast<SheetView*>(view);
            Q_ASSERT(d->sv);
            tracks = ((Sheet*)d->sv->get_sheet())->get_audio_tracks();
        } else {
            d->sv = qobject_cast<SheetView*>(view->get_sheetview());
            Q_ASSERT(d->sv);
        }

        d->doSnap = d->sv->get_sheet()->is_snap_on();

        TimeRef currentLocation = TimeRef(cpointer().on_first_input_event_scene_x() * d->sv->timeref_scalefactor);

        if (d->sv->get_audio_trackview_at_scene_pos(cpointer().scene_pos())) {
            mcd->pointedTrackIndex = d->sv->get_audio_trackview_at_scene_pos(cpointer().scene_pos())->get_track()->get_sort_index();
        } else {
            mcd->pointedTrackIndex = 0;
        }

        if (m_actionType == FOLD_SHEET || m_actionType == FOLD_MARKERS) {
            QList<Marker*> movingMarkers = d->sv->get_sheet()->get_timeline()->get_markers();
            foreach(Marker* marker, movingMarkers) {
                if (marker->get_when() > currentLocation) {
                    MarkerAndOrigin markerAndOrigin;
                    markerAndOrigin.marker = marker;
                    markerAndOrigin.origin = marker->get_when();
                    m_markers.append(markerAndOrigin);
                }
            }
        }

        if (m_actionType == FOLD_SHEET || m_actionType == FOLD_TRACK) {
            foreach(AudioTrack* track, tracks) {
                QList<AudioClip*> clips = track->get_audioclips();
                foreach(AudioClip* clip, clips) {
                    if (clip->get_track_end_location() > currentLocation) {
                        movingClips.append(clip);
                    }
                }
            }
        }

        m_group.set_clips(movingClips);

    } else {
        AudioClipView* cv = qobject_cast<AudioClipView*>(view);
        Q_ASSERT(cv);
        d->sv = cv->get_sheetview();
        AudioClip* clip  = cv->get_clip();
        if (clip->is_selected()) {
            QList<AudioClip*> selected;
            clip->get_sheet()->get_audioclip_manager()->get_selected_clips(selected);
            m_group.set_clips(selected);
        } else {
            m_group.add_clip(clip);
        }
        mcd->pointedTrackIndex = clip->get_track()->get_sort_index();
    }

    m_origTrackIndex = m_newTrackIndex = m_group.get_track_index();
    if (m_group.get_size() == 0 && m_markers.count() > 0) {
        m_trackStartLocation = m_markers[0].origin;
    } else {
        m_trackStartLocation = m_group.get_track_start_location();
    }
    m_session = d->sv->get_sheet();
    mcd->zoom = nullptr;
}

MoveClip::~MoveClip()
{
    if (mcd) {
        if (mcd->zoom) {
            delete mcd->zoom;
        }
        delete mcd;
    }
}

int MoveClip::begin_hold()
{
    if ((!m_group.get_size() || m_group.is_locked()) && !m_markers.count()) {
        return -1;
    }

    if (m_actionType == COPY) {
        // FIXME Memory leak here!
        QList<AudioClip*> newclips = m_group.copy_clips();
        m_group.set_clips(newclips);
        m_group.add_all_clips_to_tracks();
        m_group.move_to(m_origTrackIndex, m_trackStartLocation + TimeRef(d->sv->timeref_scalefactor * 3));
    }

    m_group.set_as_moving(true);

    mcd->sceneXStartPos = cpointer().on_first_input_event_scene_x();
    mcd->relativeWorkCursorPos = m_session->get_work_location() - m_group.get_track_start_location();

    d->sv->stop_follow_play_head();

    if (!cpointer().keyboard_only_input()) {
        // FIXME
        // is already called from TInputEventDispatcher but there we do
        // not check for keyboard_only_input. should we do that, what is
        // the function of that
//        MoveCommand::begin_hold();
    }

    cpointer().set_canvas_cursor_text(timeref_to_text(m_group.get_track_start_location(), d->sv->timeref_scalefactor));

    return 1;
}


int MoveClip::finish_hold()
{
    m_group.set_as_moving(false);

    return 1;
}


int MoveClip::prepare_actions()
{
    if (mcd->zoom) {
        delete mcd->zoom;
    }
    delete mcd;
    mcd = nullptr;

    if (m_actionType == COPY) {
        m_group.remove_all_clips_from_tracks();
    }

    if (m_origTrackIndex == m_newTrackIndex &&  m_posDiff == TimeRef() &&
            ! (m_actionType == COPY || m_actionType == MOVE_TO_START || m_actionType == MOVE_TO_END) ) {
        return -1;
    }

    return 1;
}


int MoveClip::do_action()
{
    PENTER;
    if (m_actionType == MOVE || m_actionType == FOLD_SHEET || m_actionType == FOLD_TRACK) {
        m_group.move_to(m_newTrackIndex, m_trackStartLocation + m_posDiff);
    }
    else if (m_actionType == COPY) {
        m_group.add_all_clips_to_tracks();
        m_group.move_to(m_newTrackIndex, m_trackStartLocation + m_posDiff);
    }
    else if (m_actionType == MOVE_TO_START) {
        move_to_start();
    }
    else if (m_actionType == MOVE_TO_END) {
        move_to_end();
    }

    foreach(MarkerAndOrigin markerAndOrigin, m_markers) {
        markerAndOrigin.marker->set_when(markerAndOrigin.origin + m_posDiff);
    }

    return 1;
}


int MoveClip::undo_action()
{
    PENTER;
    if (m_actionType == COPY) {
        m_group.remove_all_clips_from_tracks();
    } else {
        m_group.move_to(m_origTrackIndex, m_trackStartLocation);
    }

    foreach(MarkerAndOrigin markerAndOrigin, m_markers) {
        markerAndOrigin.marker->set_when(markerAndOrigin.origin);
    }

    return 1;
}

void MoveClip::cancel_action()
{
    finish_hold();
    undo_action();
}

int MoveClip::jog()
{
    if (mcd->zoom) {
        mcd->zoom->jog();
        return 0;
    }

    AudioTrackView* trackView = d->sv->get_audio_trackview_at_scene_pos(cpointer().scene_pos());
    int deltaTrackIndex = 0;
    if (trackView/* && !(m_actionType == FOLD_SHEET)*/) {
        deltaTrackIndex = trackView->get_track()->get_sort_index() - mcd->pointedTrackIndex;
        m_group.check_valid_track_index_delta(deltaTrackIndex);
        m_newTrackIndex = m_newTrackIndex + deltaTrackIndex;
        mcd->pointedTrackIndex = trackView->get_track()->get_sort_index();
    }

    // Calculate the distance moved based on the current scene x pos and the initial one.
    // Only assign if we the movements is allowed in horizontal direction
    TimeRef diff_f;
    if (!mcd->verticalOnly) {
        diff_f = (cpointer().scene_x() - mcd->sceneXStartPos) * d->sv->timeref_scalefactor;
    }

    // If the moved distance (diff_f) makes as go beyond the left most position (== 0, or TimeRef())
    // set the newTrackStartLocation to 0. Else calculate it based on the original track start location
    // and the distance moved.
    TimeRef newTrackStartLocation;
    if (diff_f < TimeRef() && m_trackStartLocation < (-1 * diff_f)) {
        newTrackStartLocation = qint64(0);
    } else {
        newTrackStartLocation = m_trackStartLocation + diff_f;
    }

    // substract the snap distance, if snap is turned on.
    if ((m_session->is_snap_on() || d->doSnap) && !mcd->verticalOnly) {
        newTrackStartLocation -= m_session->get_snap_list()->calculate_snap_diff(newTrackStartLocation, newTrackStartLocation + m_group.get_length());
    }

    // Now that the new track start location is known, the position diff can be calculated
    m_posDiff = newTrackStartLocation - m_trackStartLocation;

    // and used to move the group to it's new location.
    m_group.move_to(m_newTrackIndex, m_trackStartLocation + m_posDiff);

    // and used to move the markers
    foreach(MarkerAndOrigin markerAndOrigin, m_markers) {
        markerAndOrigin.marker->set_when(markerAndOrigin.origin + m_posDiff);
    }

    cpointer().set_canvas_cursor_text(timeref_to_text(newTrackStartLocation, d->sv->timeref_scalefactor));

    return 1;
}


void MoveClip::next_snap_pos()
{

    do_prev_next_snap(m_session->get_snap_list()->next_snap_pos(m_group.get_track_start_location()),
                      m_session->get_snap_list()->next_snap_pos(m_group.get_track_end_location()));
}

void MoveClip::prev_snap_pos()
{

    do_prev_next_snap(m_session->get_snap_list()->prev_snap_pos(m_group.get_track_start_location()),
                      m_session->get_snap_list()->prev_snap_pos(m_group.get_track_end_location()));
}

void MoveClip::do_prev_next_snap(TimeRef trackStartLocation, TimeRef trackEndLocation)
{
    if (mcd->verticalOnly) return;
    ied().bypass_jog_until_mouse_movements_exceeded_manhattenlength();
    trackStartLocation -= m_session->get_snap_list()->calculate_snap_diff(trackStartLocation, trackEndLocation);
    m_posDiff = trackStartLocation - m_trackStartLocation;
    do_move();
}

void MoveClip::move_to_start()
{

    m_group.move_to(m_group.get_track_index(), TimeRef());
}

void MoveClip::move_to_end()
{

    m_group.move_to(m_group.get_track_index(), m_session->get_last_location());
}

void MoveClip::move_up()
{
    int deltaTrackIndex = -1;
    m_group.check_valid_track_index_delta(deltaTrackIndex);
    m_newTrackIndex = m_newTrackIndex + deltaTrackIndex;
    do_move();
}

void MoveClip::move_down()
{
    int deltaTrackIndex = 1;
    m_group.check_valid_track_index_delta(deltaTrackIndex);
    m_newTrackIndex = m_newTrackIndex + deltaTrackIndex;
    do_move();
}

void MoveClip::move_left()
{
    if (mcd->zoom) {
        mcd->zoom->hzoom_out();
        return;
    }

    if (mcd->verticalOnly) return;

    if (d->doSnap) {
        return prev_snap_pos();
    }

    m_posDiff -= (d->sv->timeref_scalefactor * d->speed);
    if (m_posDiff + m_trackStartLocation < TimeRef()) {
        m_posDiff = -1 * m_trackStartLocation;
    }
    do_move();
}

void MoveClip::move_right()
{
    if (mcd->zoom) {
        mcd->zoom->hzoom_in();
        return;
    }

    if (mcd->verticalOnly) return;

    if (d->doSnap) {
        return next_snap_pos();
    }

    m_posDiff += (d->sv->timeref_scalefactor * d->speed);
    do_move();
}

void MoveClip::start_zoom()
{
    if (!mcd->zoom) {
        mcd->zoom = new Zoom(d->sv, QList<QVariant>() << "HJogZoom" << "1.2" << "0.2");
        mcd->zoom->begin_hold();
        cpointer().set_canvas_cursor_shape(":/cursorZoomHorizontal");
        // FIXME, should no longer be handled from inherited class
//        stop_shuttle();
    } else {
        cpointer().set_canvas_cursor_shape(":/cursorHoldLrud");
        // FIXME, should no longer be handled from inherited class
//        start_shuttle(true);
        delete mcd->zoom;
        mcd->zoom = 0;
    }
}

void MoveClip::toggle_vertical_only()
{
    mcd->verticalOnly = !mcd->verticalOnly;
    if (mcd->verticalOnly) {
        set_cursor_shape(0, 1);
        cpointer().set_canvas_cursor_text(tr("Vertical On"), 1000);
    } else {
        set_cursor_shape(1, 1);
        cpointer().set_canvas_cursor_text(tr("Vertical Off"), 1000);
    }
}

void MoveClip::do_move()
{       
    ied().bypass_jog_until_mouse_movements_exceeded_manhattenlength();

    m_group.move_to(m_newTrackIndex, m_trackStartLocation + m_posDiff);

    if (mcd) {
        TrackView* tv = d->sv->get_track_views().at(m_newTrackIndex);
        qreal sceneY = tv->scenePos().y() + tv->boundingRect().height() / 2;
        d->sv->keyboard_move_canvas_cursor_to_location(m_trackStartLocation + m_posDiff + mcd->relativeWorkCursorPos, sceneY);
        d->sv->set_edit_cursor_text(timeref_to_text(m_trackStartLocation + m_posDiff, d->sv->timeref_scalefactor));
    }
}

void MoveClip::set_jog_bypassed(bool bypassed)
{
    if (mcd && bypassed) {
        stop_shuttle();
    }
}
