#pragma once

enum class StatusMode {
    Connecting,
    Offline,
    Connected,
    UnlockSuccess,
    UnlockFail,
    Lock
};

void setStatus(StatusMode mode);
void updateStatus();
void initStatus();