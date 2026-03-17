#pragma once

enum class StatusMode {
    Connecting,
    Offline,
    Connected,
    UnlockSuccess,
    UnlockFail,
    Lock,
    Unlock
};

void setStatus(StatusMode mode);
void updateStatus();
void initStatus();