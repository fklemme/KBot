#pragma once

namespace KBot {

    class KBot;

    class Manager {
    public:
        Manager(KBot &parent);

        // Is called every KBot::onFrame().
        void update();

    private:
        KBot &m_kBot;
    };

} // namespace
