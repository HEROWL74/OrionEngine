# OrionEngine ==> Game Engine Prototype

![Build and Release](https://github.com/HEROWL74/DX12GameEngine/actions/workflows/release.yml/badge.svg)

自作ゲームエンジン「OrionEngine」の開発プロジェクトです。  
DirectX12とC++を用いて、レンダリング・UI・スクリプト実行基盤までを一貫して設計・実装しています。  
実際に本エンジン上でミニゲームを制作し、リリースビルドまで行っています。
---

## 🎯 目的とビジョン

- ゲームエンジンの構造（Scene, Entity, Component, Rendererなど）を理解する
- UnityやUnreal Engineのような再利用性の高いフレームワークを目指す
- 将来的には **教育用エンジン**として使えるようにすることを目指す
---

## 🎯 想定ユーザー

- ゲームエンジンの仕組みを学びたい開発者
- 小規模ゲームを高速に試作したい個人開発者

## ✨ 現在の進捗 OrionEngine 第一作ミニゲーム完成
https://github.com/HEROWL74/OrionEngine/releases/tag/GameVol.1

### エディタ画像
![image](https://github.com/user-attachments/assets/66efdfdc-a671-4e18-ba63-b5c6466276ba)
### 作成したミニゲームスクリーンショット
![image](https://github.com/user-attachments/assets/e82ae3d1-695b-499d-b06b-5fa8d6067d2e)

-✅ 完成済み

- 最低限のレンダリング基盤
- BoxCollider, InputSystem, AudioComponent, のLuaスクリプトでの制御
- シェーダーシステム
- EditorGUIでのオブジェクト操作
- 制作したゲームのリリースビルド機能

---

## ✨ 技術的な特徴

- DirectX12の抽象化  
  → PSOやRootSignatureをラップし、低レイヤの複雑さを隠蔽

- Luaスクリプトによる高速なゲームロジック開発  
  → コンパイル不要で反復開発が可能

- エディタ統合開発環境  
  → ImGuiベースでパラメータ編集・即時反映

- リリースビルド機能  
  → エンジン上で制作したゲームをそのまま配布可能
---

## 🧱 ディレクトリ構造と設計意図

プロジェクト全体の責務を明確に分けるため、以下のディレクトリ設計を採用しています。

```text
OrionEngine/
├── engine/             # エンジンコア・フレームワーク層
│   ├── Core/           # アプリケーション基盤・GameObject・Scene管理
├── renderer/           # DirectX12 ラッパー (Device, SwapChain, PSO管理)
├── runtime/            # ゲーム実行用バイナリ & DLL差替システムの実装
├── editor/             # ImGuiベースのエディタ機能と専用描画ロジック
├── tools/              # ビルド支援ツール (BuildWorker等)
├── assets/             # ユーザー資産 (モデル、テクスチャ、Luaスクリプト)
└── engine-assets/      # エンジン内蔵資産 (標準シェーダー、ビルド済みUI素材)
```

## 💡 工夫した点

- ディレクトリ構成を再設計し、engine / runtime / editor の責務分離を明確化
- GPU非同期処理の問題を修正し、安定した描画処理を実現
- ビルドフローを改善し、リリースビルドを簡単に生成できるようにした

---


## 🙌 このプロジェクトについて

高校生として、将来ゲームエンジニアになるために、基礎から丁寧に制作中です。

## 📧 作者について

- 名前：HEROWL
- X (旧Twitter)：[HRAKProgrammer](https://x.com/HRAKProgrammer)
- 開発環境：Visual Studio 2022 / Windows 11 / C++20&23
- 興味分野：ゲームエンジン開発、ゲーム開発、バスケットボール、水上オートバイ運転
