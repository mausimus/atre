#pragma once

#include "atre.hpp"

namespace atre
{
class Atari;

class Callbacks
{
public:
    virtual void OnTrap() {}
    virtual void OnIRQ() {}
    virtual void OnNMI() {}
    virtual void OnBRK() {}
    virtual void OnReset() {}
    virtual void OnBreak() {}
    virtual void DumpState() {}

    virtual ~Callbacks() {}
};

class Debugger : public Callbacks
{
public:
    Debugger(Atari* atari);

    void Initialize();
    void Exit();

    void OnTrap() override;
    void OnIRQ() override;
    void OnNMI() override;
    void OnBRK() override;
    void OnReset() override;
    void OnBreak() override;
    void DumpState() override;

    // commands
    void Start();
    void Stop();
    void CallStack();
    void Steps(bool);
    void DumpRAM(const std::string& fileName);
    void ShowDList();

private:
    Atari*                       m_atari;
    std::atomic_bool             m_exiting;
    std::atomic_bool             m_stopping;
    std::mutex                   m_mutex;
    std::condition_variable      m_running;
    std::unique_ptr<std::thread> m_CPUThread;
    std::unique_ptr<std::thread> m_IOThread;

    void CPUThread();
    void IOThread();
};
} // namespace atre
