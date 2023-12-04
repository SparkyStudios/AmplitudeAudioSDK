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

#include <SparkyStudios/Audio/Amplitude/Math/Curve.h>

#include <SparkyStudios/Audio/Amplitude/Sound/Rtpc.h>

#include <Core/EngineInternalState.h>

#include "rtpc_definition_generated.h"

namespace SparkyStudios::Audio::Amplitude
{
    Rtpc::Rtpc()
        : _name()
        , _minValue(0.0)
        , _maxValue(1.0)
        , _defValue(0.0)
        , _currentValue(0.0)
        , _targetValue(0.0)
        , _faderAttackFactory(nullptr)
        , _faderReleaseFactory(nullptr)
        , _faderAttack(nullptr)
        , _faderRelease(nullptr)
    {}

    Rtpc::~Rtpc()
    {
        if (_faderAttack != nullptr)
            _faderAttackFactory->DestroyInstance(_faderAttack);

        if (_faderRelease != nullptr)
            _faderReleaseFactory->DestroyInstance(_faderRelease);

        _faderAttackFactory = nullptr;
        _faderReleaseFactory = nullptr;

        _faderAttack = nullptr;
        _faderRelease = nullptr;

        _name.clear();
    }

    void Rtpc::Update(AmTime deltaTime)
    {
        if (_faderRelease && _currentValue > _targetValue)
        {
            _currentValue = _faderRelease->GetFromTime(Engine::GetInstance()->GetTotalTime());
        }

        if (_faderAttack && _currentValue < _targetValue)
        {
            _currentValue = _faderAttack->GetFromTime(Engine::GetInstance()->GetTotalTime());
        }
    }

    double Rtpc::GetMinValue() const
    {
        return _minValue;
    }

    double Rtpc::GetMaxValue() const
    {
        return _maxValue;
    }

    double Rtpc::GetValue() const
    {
        return _currentValue;
    }

    void Rtpc::SetValue(AmReal64 value)
    {
        _targetValue = AM_CLAMP(value, _minValue, _maxValue);

        if (_faderAttack == nullptr || _faderRelease == nullptr)
        {
            _currentValue = value;
        }
        else
        {
            _faderAttack->Set(_currentValue, _targetValue);
            _faderRelease->Set(_currentValue, _targetValue);

            _faderAttack->Start(Engine::GetInstance()->GetTotalTime());
            _faderRelease->Start(Engine::GetInstance()->GetTotalTime());
        }
    }

    AmReal64 Rtpc::GetDefaultValue() const
    {
        return _defValue;
    }

    void Rtpc::Reset()
    {
        SetValue(_defValue);
    }

    bool Rtpc::LoadDefinition(const RtpcDefinition* definition, EngineInternalState* state)
    {
        _id = definition->id();
        _name = definition->name()->str();

        _minValue = definition->min_value();
        _maxValue = definition->max_value();
        _defValue = definition->default_value();

        if (definition->fade_settings() && definition->fade_settings()->enabled())
        {
            _faderAttackFactory = Fader::Find(definition->fade_settings()->fade_attack()->fader()->str());
            _faderReleaseFactory = Fader::Find(definition->fade_settings()->fade_release()->fader()->str());

            if (_faderAttackFactory != nullptr)
            {
                _faderAttack = _faderAttackFactory->CreateInstance();
                _faderAttack->SetDuration(definition->fade_settings()->fade_attack()->duration());
            }

            if (_faderReleaseFactory != nullptr)
            {
                _faderRelease = _faderReleaseFactory->CreateInstance();
                _faderRelease->SetDuration(definition->fade_settings()->fade_release()->duration());
            }
        }

        return true;
    }

    const RtpcDefinition* Rtpc::GetDefinition() const
    {
        return GetRtpcDefinition(_source.c_str());
    }

    RtpcValue::RtpcValue()
        : _valueKind(ValueKind_None)
        , _value(0.0f)
        , _curve(nullptr)
        , _ownCurve(false)
        , _rtpc(nullptr)
    {}

    RtpcValue::RtpcValue(AmReal32 value)
        : _valueKind(ValueKind_Static)
        , _value(value)
        , _curve(nullptr)
        , _ownCurve(false)
        , _rtpc(nullptr)
    {}

    RtpcValue::RtpcValue(const Rtpc* rtpc, Curve* curve)
        : _valueKind(ValueKind_RTPC)
        , _value(0.0f)
        , _curve(curve)
        , _ownCurve(false)
        , _rtpc(rtpc)
    {}

    RtpcValue::RtpcValue(const RtpcCompatibleValue* definition)
        : _valueKind(definition->kind())
        , _value(definition->value())
        , _curve(nullptr)
        , _ownCurve(true)
        , _rtpc(nullptr)
    {
        if (definition->kind() == ValueKind_RTPC)
        {
            _rtpc = amEngine->GetRtpcHandle(definition->rtpc()->id());

            if (_rtpc == nullptr)
            {
                CallLogFunc("[ERROR] Linking a parameter to an invalid or uninitialized RTPC handle.");
            }

            auto* curve = ampoolnew(MemoryPoolKind::Engine, Curve);
            curve->Initialize(definition->rtpc()->curve());

            _curve = curve;
        }
    }

    RtpcValue::~RtpcValue()
    {
        if (_ownCurve && _curve != nullptr)
            ampooldelete(MemoryPoolKind::Engine, Curve, _curve);

        _ownCurve = false;
        _curve = nullptr;
        _rtpc = nullptr;
    }

    float RtpcValue::GetValue() const
    {
        if (_valueKind == ValueKind_RTPC && _rtpc != nullptr)
            return _curve->Get(_rtpc->GetValue());

        if (_valueKind == ValueKind_Static)
            return _value;

        return 0.0f;
    }
} // namespace SparkyStudios::Audio::Amplitude
