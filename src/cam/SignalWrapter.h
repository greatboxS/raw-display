#ifndef SIGNALWRAPTER_H
#define SIGNALWRAPTER_H

#include <condition_variable>
#include <mutex>

namespace evs {
namespace early {

typedef struct ConditionVariableWrapter_t {
    std::condition_variable condition; ///< Condition variable for signaling
    std::mutex mutex;                  ///< Mutex for synchronizing access
    int count = 0;                     ///< Count of signals sent
} ConditionVariableWrapter;

/**
 * @class SignalWrapter
 * @brief Wraps a condition variable and mutex for signaling between threads.
 *        This class provides methods to create, destroy, wait for, and notify signals.
 */
class SignalWrapter
{

    SignalWrapter(const SignalWrapter &) = delete;
    SignalWrapter &operator=(const SignalWrapter &) = delete;
    SignalWrapter(SignalWrapter &&other) = delete;
    SignalWrapter &operator=(SignalWrapter &&other) = delete;

public:
    /**
     * @brief Default constructor.
     * Initializes a new SignalWrapter with a ConditionVariableWrapter.
     */
    SignalWrapter()
        : m_wrapter(nullptr) {
    }

    /**
     * @brief Destructor.
     * Cleans up the ConditionVariableWrapter.
     * Deletes the internal ConditionVariableWrapter pointer.
     */
    ~SignalWrapter() {
        destroySignal();
    }

    inline bool isCreated() {
        return (m_wrapter != nullptr);
    }

    /**
     * @brief Destroys the signal, resetting the count to zero.
     * @return 0 on success, or an error code if the signal cannot be destroyed.
     */
    inline int createSignal() {
        int ret = 0;
        do {
            if (m_wrapter != nullptr) {
                ret = -1;
                break;
            }
            m_wrapter = new (std::nothrow) ConditionVariableWrapter();

            if (m_wrapter == nullptr) {
                ret = -2;
                break;
            }

            std::lock_guard<std::mutex> lock(m_wrapter->mutex);
            m_wrapter->count = 0;
        } while (false);

        return ret;
    }

    /**
     * @brief Destroys the signal, resetting the count to zero.
     * @return 0 on success, or an error code if the signal cannot be destroyed.
     */
    inline int destroySignal() {
        int ret = 0;
        do {
            if (m_wrapter == nullptr) {
                ret = -1;
                break;
            }
            delete m_wrapter;
            m_wrapter = nullptr;
        } while (false);
        return ret;
    }

    /**
     * @brief Waits for a signal to be set, blocking until a signal is received.
     * @param cvWrapper Pointer to the ConditionVariableWrapter.
     * @return 0 on success, or an error code if the wait fails.
     */
    inline int waitForSignal(long long timeout = -1) {
        int ret = 0;
        bool isTimeout = false;

        do {
            if (m_wrapter == nullptr) {
                ret = -1;
                break;
            }

            if (timeout >= 0) {

                std::unique_lock<std::mutex> lock(m_wrapter->mutex);
                isTimeout = m_wrapter->condition.wait_for(lock, std::chrono::milliseconds(timeout), [this]() {
                    return m_wrapter->count > 0;
                });

                if (isTimeout == false) {
                    m_wrapter->count -= 1;
                    ret = 0;
                } else {
                    ret = -2;
                    break;
                }
            } else {
                std::unique_lock<std::mutex> lock(m_wrapter->mutex);
                m_wrapter->condition.wait(lock, [this]() {
                    return m_wrapter->count > 0;
                });
                m_wrapter->count -= 1;
            }
        } while (false);
        return ret;
    }

    template <typename Fnc>
    inline int waitForSignal(long long timeout = -1 , Fnc&&callback = nullptr) {
        int ret = 0;
        bool isTimeout = false;

        do {
            if (m_wrapter == nullptr) {
                ret = -1;
                break;
            }

            if (timeout >= 0) {

                std::unique_lock<std::mutex> lock(m_wrapter->mutex);
                isTimeout = m_wrapter->condition.wait_for(lock, std::chrono::milliseconds(timeout), [this, callback]() {
                    return ((m_wrapter->count > 0) || ((callback != nullptr) && callback()));
                });

                if (isTimeout == false) {
                    m_wrapter->count -= 1;
                    ret = 0;
                } else {
                    ret = -2;
                    break;
                }
            } else {
                std::unique_lock<std::mutex> lock(m_wrapter->mutex);
                m_wrapter->condition.wait(lock, [this, callback]() {
                    return ((m_wrapter->count > 0) || ((callback != nullptr) && callback()));
                });
                m_wrapter->count -= 1;
            }
        } while (false);
        return ret;
    }

    /**
     * @brief Sets a signal, notifying one waiting thread.
     * @param m_wrapter Pointer to the ConditionVariableWrapter.
     * @return 0 on success, or an error code if the signal cannot be set.
     */
    inline int notifySignal() {
        int ret = 0;
        do {
            if (m_wrapter == nullptr) {
                ret = -1;
                break;
            }

            std::lock_guard<std::mutex> lock(m_wrapter->mutex);
            m_wrapter->count += 1;
            m_wrapter->condition.notify_all();
        } while (false);
        return ret;
    }

private:
    ConditionVariableWrapter *m_wrapter{nullptr};
};

} // namespace early
} // namespace evs

#endif // SIGNALWRAPTER_H