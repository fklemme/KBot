#pragma once

namespace KBot {

    class KBot;

    class General {
    public:
        General(KBot &parent);

        // Is called every KBot::onFrame().
        void update();

    private:
        KBot &m_kBot;
    };

} // namespace
