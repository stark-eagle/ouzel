// Copyright 2015-2018 Elviss Strazdins. All rights reserved.

#include "Delay.hpp"
#include "Audio.hpp"

namespace ouzel
{
    namespace audio
    {
        class DelayProcessor final: public Processor
        {
        public:
            DelayProcessor()
            {
            }

            void process(uint32_t frames, uint16_t channels, uint32_t sampleRate,
                         std::vector<float>& samples) override
            {
            }
        };

        Delay::Delay(Audio& initAudio):
            Filter(initAudio,
                   initAudio.initProcessor(std::unique_ptr<Processor>(new DelayProcessor())))
        {
        }

        Delay::~Delay()
        {
        }

        void Delay::setDelay(float newDelay)
        {
            delay = newDelay;
        }
    } // namespace audio
} // namespace ouzel
