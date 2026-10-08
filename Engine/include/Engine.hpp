//
// Created by genin on 26/01/2026.
// Path: Engine/include/Engine.hpp
//

// Umbrella header: the whole public API in one include, for quick prototypes.
// Engine code and real games should include only what they use, which keeps
// compile times and dependencies visible.
//

#pragma once

// Core
#include "Engine/Core/Assert.hpp"
#include "Engine/Core/Base.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/FileSystem.hpp"
#include "Engine/Core/Input.hpp"
#include "Engine/Core/KeyCodes.hpp"
#include "Engine/Core/Log.hpp"
#include "Engine/Core/Window.hpp"

// Events
#include "Engine/Event/ApplicationEvent.hpp"
#include "Engine/Event/EventBus.hpp"

// Renderer (API-independent layer)
#include "Engine/Renderer/Buffer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/CommandList.hpp"
#include "Engine/Renderer/GraphicsDevice.hpp"
#include "Engine/Renderer/Handle.hpp"
#include "Engine/Renderer/LightingData.hpp"
#include "Engine/Renderer/Mesh.hpp"

// Assets
#include "Engine/Assets/ObjLoader.hpp"

// Scene (ECS components and systems)
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/SceneRenderer.hpp"
