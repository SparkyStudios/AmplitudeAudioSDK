// Copyright (c) 2021-present Sparky Studios. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

#include <Core/EngineInternalState.h>
#include <Core/EntityInternalState.h>

#include "event_definition_generated.h"
#include "sound_collection_definition_generated.h"

namespace SparkyStudios::Audio::Amplitude
{
    EventAction::EventAction()
        : EventAction(nullptr)
    {}

    EventAction::EventAction(Event* parent)
        : _parent(parent)
        , _active(false)
        , _type(EventActionType_None)
        , _targets()
        , _scope(EventActionScope_Entity)
    {}

    void EventAction::Initialize(const EventActionDefinition* definition)
    {
        _active = definition->active();
        _type = definition->type();
        _scope = definition->scope();
        flatbuffers::uoffset_t targets_count = definition->targets() ? definition->targets()->size() : 0;
        for (flatbuffers::uoffset_t i = 0; i < targets_count; ++i)
        {
            _targets.push_back(definition->targets()->Get(i)->c_str());
        }
    }

    void EventAction::Run(const Entity& entity)
    {
        if (!_active)
            return;

        switch (_type)
        {
        default:
        case EventActionType_None:
            return;

        case EventActionType_Play:
            return _executePlay(entity);

        case EventActionType_Pause:
            return _executePause(entity);

        case EventActionType_Resume:
            return _executeResume(entity);

        case EventActionType_Stop:
            return _executeStop(entity);

        case EventActionType_Seek:
            return _executeSeek(entity);

        case EventActionType_Mute:
            return _executeMute(entity, true);

        case EventActionType_Unmute:
            return _executeMute(entity, false);
        }
    }

    bool EventAction::AdvanceFrame(AmTime delta_time)
    {
        // TODO
        return false;
    }

    void EventAction::_executePlay(const Entity& entity)
    {
        auto* engine = Engine::GetInstance();

        for (auto&& target : _targets)
        {
            engine->Play(target, entity);
        }
    }

    void EventAction::_executePause(const Entity& entity)
    {
        auto* engine = Engine::GetInstance();

        for (auto&& target : _targets)
        {
            if (_scope == EventActionScope_Entity)
            {
                for (auto&& item : entity.GetState()->GetPlayingSoundList())
                {
                    if (strcmp(target, item.GetSoundCollection()->GetSoundCollectionDefinition()->name()->c_str()) == 0)
                    {
                        item.Pause();
                    }
                }
            }
            else
            {
                // TODO
            }
        }
    }

    void EventAction::_executeResume(const Entity& entity)
    {
        auto* engine = Engine::GetInstance();

        for (auto&& target : _targets)
        {
            if (_scope == EventActionScope_Entity)
            {
                for (auto&& item : entity.GetState()->GetPlayingSoundList())
                {
                    if (strcmp(target, item.GetSoundCollection()->GetSoundCollectionDefinition()->name()->c_str()) == 0)
                    {
                        item.Resume();
                    }
                }
            }
            else
            {
                // TODO
            }
        }
    }

    void EventAction::_executeStop(const Entity& entity)
    {
        auto* engine = Engine::GetInstance();

        for (auto&& target : _targets)
        {
            if (_scope == EventActionScope_Entity)
            {
                for (auto&& item : entity.GetState()->GetPlayingSoundList())
                {
                    if (strcmp(target, item.GetSoundCollection()->GetSoundCollectionDefinition()->name()->c_str()) == 0)
                    {
                        item.Halt();
                    }
                }
            }
            else
            {
                // TODO
            }
        }
    }

    void EventAction::_executeSeek(const Entity& entity)
    {}

    void EventAction::_executeMute(const Entity& entity, bool mute)
    {
        auto* engine = Engine::GetInstance();

        for (auto&& target : _targets)
        {
            Bus bus = engine->FindBus(target);
            if (bus.Valid())
            {
                bus.SetMute(mute);
            }
        }
    }

    bool Event::LoadEventDefinition(const std::string& event)
    {
        _source = event;
        const EventDefinition* definition = GetEventDefinition();
        _id = definition->id();
        _name = definition->name()->c_str();
        flatbuffers::uoffset_t actions_count = definition->actions() ? definition->actions()->size() : 0;
        _actions.resize(actions_count);
        for (flatbuffers::uoffset_t i = 0; i < actions_count; ++i)
        {
            const EventActionDefinition* item = definition->actions()->Get(i);

            EventAction& action = _actions[i];
            action._parent = this;
            action.Initialize(item);
        }

        return true;
    }

    bool Event::LoadEventDefinitionFromFile(AmOsString filename)
    {
        std::string source;
        return LoadFile(filename, &source) && LoadEventDefinition(source);
    }

    EventInstance Event::Trigger(const Entity& entity)
    {
        CallLogFunc("[Debug] Event %s triggered.\n", _name);

        auto event = EventInstance(this);
        event.Start(entity);

        return event;
    }

    AmUInt64 Event::GetId() const
    {
        return _id;
    }

    AmString Event::GetName() const
    {
        return _name;
    }

    const EventDefinition* Event::GetEventDefinition() const
    {
        return Amplitude::GetEventDefinition(_source.c_str());
    }

    RefCounter* Event::GetRefCounter()
    {
        return &_refCounter;
    }

    EventCanceler::EventCanceler()
        : _event(nullptr)
    {}

    EventCanceler::EventCanceler(EventInstance* event)
        : _event(event)
    {}

    bool EventCanceler::Valid() const
    {
        return _event != nullptr;
    }

    void EventCanceler::Cancel()
    {
        AMPLITUDE_ASSERT(Valid());
        _event->Abort();
    }

    EventInstance* EventCanceler::GetEvent() const
    {
        return _event;
    }

    EventInstance::EventInstance()
        : _running(false)
        , _actions()
    {}

    EventInstance::EventInstance(Event* parent)
        : _running(false)
        , _actions(parent->_actions)
    {}

    void EventInstance::Start(const Entity& entity)
    {
        _running = true;
        for (auto&& action : _actions)
        {
            action.Run(entity);
        }
    }

    void EventInstance::AdvanceFrame(AmTime delta_time)
    {
        if (!_running)
            return;

        _running = false;
        for (auto&& action : _actions)
        {
            _running |= action.AdvanceFrame(delta_time);
        }
    }

    bool EventInstance::IsRunning() const
    {
        return _running;
    }

    void EventInstance::Abort()
    {
        _running = false;
    }
} // namespace SparkyStudios::Audio::Amplitude