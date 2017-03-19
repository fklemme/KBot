#pragma once

namespace KBot {

    class KBot;

    class Base {
    public:
        Base(KBot &kBot);

    private:
        KBot &m_kBot;
    };

} // namespace
