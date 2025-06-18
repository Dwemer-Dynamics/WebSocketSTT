#ifndef VOICE_RECORD_CONTROL_H
#define VOICE_RECORD_CONTROL_H

class VoiceRecordControl {
private:
    // Private constructor to prevent external instantiation
    VoiceRecordControl() : boolValue(false) {}

    // Private copy constructor and assignment operator to prevent cloning
    VoiceRecordControl(const VoiceRecordControl&) = delete;
    VoiceRecordControl& operator=(const VoiceRecordControl&) = delete;

    // Private data member to hold the boolean variable
    bool boolValue;

    // Private mutex for thread safety
    std::mutex mutex_;

public:
    // Public static method to access the singleton instance
    static VoiceRecordControl& getInstance() {
        static VoiceRecordControl instance;
        return instance;
    }

    // Getter method for the boolean variable
    bool getRecording() {
        std::lock_guard<std::mutex> lock(mutex_);
        return boolValue;
    }

    // Setter method for the boolean variable
    void setRecording(bool value) {
        std::lock_guard<std::mutex> lock(mutex_);
        boolValue = value;
    }
};

#endif  // VOICE_RECORD_CONTROL_H