#pragma once

#include "adapters/media/media_observer.h"

#include <memory>

namespace plasma_bridge::media
{

class MprisMediaObserver final : public MediaObserver
{
    Q_OBJECT

public:
    explicit MprisMediaObserver(QObject *parent = nullptr);
    ~MprisMediaObserver() override;

    void start() override;
    const plasma_bridge::MediaState &currentState() const override;
    bool hasInitialState() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace plasma_bridge::media
