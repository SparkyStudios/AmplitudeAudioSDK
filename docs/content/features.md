+++
title = "Features"
description = "Browse the main features of the Amplitude Audio SDK and discover why this open-source library will be useful for your next game."
+++

{{< block >}}
{{< column "mt-2" >}}

## Summary

- [Summary](#summary)
- [Data-driven Development](#data-driven-development)
- [Drivers and Codecs](#drivers-and-codecs)
- [Customizable Pipeline](#customizable-pipeline)
- [Soundbanks Management](#soundbanks-management)
- [Sound Objects](#sound-objects)
- [Game and Engine Synchronization](#game-and-engine-synchronization)
- [Game Entities](#game-entities)
- [Listeners](#listeners)
- [Customizable Attenuation Models](#customizable-attenuation-models)
- [Sound Effects](#sound-effects)
- [Sound Events](#sound-events)
- [Buses Management and Auto-ducking](#buses-management-and-auto-ducking)
- [Channel Priority](#channel-priority)
- [ADPCM Compression and sample rate conversion](#adpcm-compression-and-sample-rate-conversion)

{{< /column >}}

{{< column "mt-2" >}}

## Data-driven Development

Amplitude uses `.json` files to define everything, from a simple sound file to a complex switch container. Each `.json` file is in a dedicated directory according to its usage (`events`, `effects`, `sounds`, `collections`, etc.). Amplitude will compile them into binary files, which can now be registered into [soundbanks](#soundbanks-management) and loaded in the game.

{{< /column >}}

{{< column "mt-1" >}}

## Drivers and Codecs

Amplitude allows you to have total control over how to communicate with the physical audio device (using an [Driver] implementation) and how to decode/encode audio samples (using [Codec] implementations). The library provides you a default [Driver] implementation using [MiniAudio](http://miniaud.io) for cross-platform support, and by default decodes FLAC, MP3, OGG, and WAV audio samples.

{{< /column >}}

{{< column "mt-1" >}}

## Customizable Pipeline

The Amplimix Pipeline define how and when Amplitude process effects, environmental sounds, obstruction, occlusion, and more. With Amplitude, you can create your own custom Pipeline to tweak how the engine is processing the audio. You have the ability to create and register custom processors in the Engine using the [Pipeline] API.

{{< /column >}}

{{< column "mt-1" >}}

## Soundbanks Management

A soundbank is a unit where all the data you need for your game is loaded. The choice remains to you on how you prefer to organize soundbanks. For example, it can be per level soundbanks (`level1.ambank`) or per-kind soundbanks (`weapon-fires.ambank`, `explosions.ambank`). Each sound object loaded from soundbanks is reference counted, so even if you load more than one soundbank with the same sound object, that one will be loaded only once.

{{< /column >}}

{{< column "mt-1" >}}

## Sound Objects

Amplitude supports a variety of sound objects which have different properties and use cases:
- [Sound]: The most basic sound object. It represents a single audio file played in the game.
- [Collection]: The collection is, like its name says, a Collection of Sounds. The collection has the particularity to pick and play through its [Scheduler] a single Sound each time it receives a play request.
- [SwitchContainer]: The switch container mixes and play Sounds or Collections according to switch states. Objects registered inside a switch container for a specific switch state are played only when this switch state is active. Unlike Collections, switch containers always play all sound objects registered for the active switch state.

{{< /column >}}

{{< column "mt-1" >}}

## Game and Engine Synchronization

You can synchronize the Amplitude [Engine] with your game state using two approaches:
- [RTPC]: RTPC stands for Real-time Parameter Control. It allows you to adjust values in the Amplitude Engine (`gain`, `priority`, `effect parameters`, etc.) from the game at runtime. For example, think about a character who climbs a mountain. You can create an RTPC which synchronizes the player altitude (the distance from the ground) with the Amplitude Engine to slightly fade between environment sounds, from a calm river (at the ground) to strong winds (at the summit).
- [Switch]: A switch is helpful in cases where you want to let the Amplitude Engine know about one state at a time. For example, your character is walking in a forest. A switch can be applied to the terrain to let the Amplitude Engine know about the surface the character is walking on, and then play the right footstep sound.

{{< /column >}}

{{< column "mt-1" >}}

## Game Entities

Amplitude has been designed for games and game engines in mind. For that approach, the Engine can link a sound object to an entity of your game, so each entity, even if they are triggering the same sound object, can behave differently and maintain a different state.

{{< /column >}}

{{< column "mt-1" >}}

## Listeners

A [Listener] is a special game entity allowing you to render positional sounds. When positional audio is playing, its volume depends on its distance from the nearest Listener and the configured [attenuation model](#customizable-attenuation-models). If there are no active Listeners, then all positional audio will be silent.

{{< /column >}}

{{< column "mt-1" >}}

## Customizable Attenuation Models

You can create an infinite variety of attenuation models for spatial sounds in Amplitude to reduce/increase the gain according to the distance from the [Listener]. You have total control over the attenuation curve settings, including curve transitions. Each attenuation model is based on one attenuation shape between the following list:
- Cone
- Sphere
- Box
- Capsule

{{< /column >}}

{{< column "mt-1" >}}

## Sound Effects

You can apply a variety of effects to your sound objects. An effect is configured through a `.json` file and consist of an effect type and a set of parameters (an array of float values). The available list of effect types are:
- [BassBoost]
- [Echo]
- [Equalizer]
- [Flanger]
- [Freeverb]
- [Robotize]

{{< /column >}}

{{< column "mt-1" >}}

## Sound Events

Sound events are a way to trigger a set of actions in the game at runtime. For example, you could trigger a set of sound to play and pause after the player has entered a room. You can add many actions in your event as needed, and each action is of a specific type from the list below:
- **None**: Just do nothing.
- **Play**: Play sound objects, you can specify the list of sound objects to play using their ID.
- **Pause**: Pause sound objects, you can specify the list of sound objects to pause using their ID.
- **Resume**: Resume sound objects previously paused, you can specify the list of sound objects to resume using their ID.
- **Stop**: Stop sound objects, you can specify the list of sound objects to stop using their ID. Already stopped sound objects will be skipped.
- **Seek**: Seeks sound objects playback to the specified position. You can specify the list of sound objects to seek using their ID.
- **MuteBus**: Completely mute a bus. You can specify the list of buses to mute using their ID.
- **UnmuteBus**: Unmute previously muted buses. You can specify the list of buses to unmute using their ID.

{{< /column >}}

{{< column "mt-1" >}}

## Buses Management and Auto-ducking

When you play a sound object, a channel is produced. That channel is sent to a bus and then processed by the mixer. Buses can interact together in a way that a bus affect the gain of another one. If there is an important sound or voice over that needs to be heard, audio playing on less important buses can be ducked to ensure the player hears what they need.

{{< /column >}}

{{< column "mt-1" >}}

## Channel Priority

In your game you may have multiple sound object playing at the same time, which can be more than what the game can handle. Amplitude will manage the priority of each audio channel and only drop the least important channels when too many simultaneous streams play at once. The number of simultaneous active and virtual channels can be configured for the Engine.

{{< /column >}}

{{< column "mt-1" >}}

## ADPCM Compression and sample rate conversion

Amplitude gives you a command line tool called **amac** (**Am**plitude **A**udio **C**ompressor). It allows to compress an audio sample with an high-quality ADPCM compression, and can optionally convert the sample rate. It's highly recommended to use **amac** when releasing a project running Amplitude.

{{< /column >}}

{{< /block >}}
