#pragma once

#include "common/media_state.h"

#include <QObject>

namespace plasma_bridge::media
{

class MediaObserver : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~MediaObserver() override = default;

    virtual void start() = 0;
    virtual const plasma_bridge::MediaState &currentState() const = 0;
    virtual bool hasInitialState() const = 0;

signals:
    void initialStateReady();
    void mediaStateChanged(const QString &reason, const QString &playerId);
    void connectionFailed(const QString &message);
};

} // namespace plasma_bridge::media
