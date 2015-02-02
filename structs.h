/*

Hanghish
Copyright (C) 2015 Daniele Rogora

This file is part of Hangish.

Hangish is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Hangish is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>

*/


#ifndef STRUCTS_H
#define STRUCTS_H

#include "qtimports.h"

enum ConversationType {
    STICKY_ONE_TO_ONE = 1,
    GROUP = 2
};

enum NotificationLevel {
    UNKNOWN = 0,
    QUIET   = 10,
    RING    = 30
};

enum FocusStatus {
    FOCUSED     = 1,
    UNFOCUSED   = 2
};

enum ActiveClientState {
    NO_ACTIVE_CLIENT        = 0,
    IS_ACTIVE_CLIENT        = 1,
    OTHER_CLIENT_IS_ACTIVE  = 2
};

enum TypingStatus {
    TYPING  = 1,
    PAUSED  = 2,
    STOPPED = 3
};

struct InitialData {
    QString self_entity;
    QString entities;
    QString conversation_states;
    QString consersation_participants;
    QString sync_timestamp;
};

struct Identity {
    QString chat_id;
    QString gaia_id;
};

struct ReadState {
    Identity userid;
    QDateTime last_read;
    QString convId;
};


struct User {
    QString chat_id;
    QString gaia_id;
    QString display_name;
    QString first_name;
    QString photo;
    QString email;
    bool alreadyParsed;
};

struct Participant {
    User user;
    QDateTime last_read_timestamp;
};

struct EventAttachmentSegment {
    int type;
    QString fullImage;
    QString previewImage;
};

struct EventValueSegment {
    int type;
    QString value;
};

struct EventValue {
    bool valid;
    QList<EventValueSegment> segments;
    QList<EventAttachmentSegment> attachments;
};

struct Event {
    QString conversationId;
    EventValue value;
    Identity sender;
    QDateTime timestamp;
    int notificationLevel;
};

struct ContinuationToken {
    QDateTime timestamp;
    QString event_id;
    QString storage_continuation_token;
};

struct ConversationState {
    QString id;
    ContinuationToken continuationToken;
};

struct Conversation {
    QString id;
    QDateTime creation_ts;
    int unread;
    QDateTime lastReadTimestamp;
    ConversationType type;
    QString name;
    User creator;
    QList<Participant> participants;
    QList<Event> events;
    QString last_ts;
    ConversationState state;
};

struct OutgoingImage {
    QString filename;
    QString conversationId;
};

#endif // STRUCTS_H
