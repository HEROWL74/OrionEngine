//src/Core/EditorState.hpp
#pragma once
#include <functional>
namespace Editor::EditorCore
{
   //エディタの実行状態
	enum class EditorState
	{
		Edit,    //編集モード（通常状態）
		Playing, //実行中
		Paused   //一時停止中
	};

	//エディタ状態変更時のコールバック
	using EditorStateChangedCallback = std::function<void(EditorState oldState, EditorState newState)>;
}

