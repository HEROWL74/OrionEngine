# OrionEngine ==> Game Engine Prototype

![Build and Release](https://github.com/HEROWL74/DX12GameEngine/actions/workflows/release.yml/badge.svg)

このプロジェクトは、高校生が独学で開発している**GameEngine**のプロトタイプです。  
C++ と DirectX12 を使って、エンジン内部の仕組みを一から学びながら構築しています。
Microsoftのリファレンスページや、サンプルコードを見ながら勉強しながらコーディングしています。
---

## 🎯 目的とビジョン

- ゲームエンジンの構造（Scene, Entity, Component, Rendererなど）を理解する
- UnityやUnreal Engineのような再利用性の高いフレームワークを目指す
- 将来的には **簡易エディタ** や **スクリプト連携** も組み込む予定

---



## ✨ 現在の進捗 Lua統合&Playmode実装中
![Luaでオブジェクトを操作]https://github.com/user-attachments/assets/7766e600-8dac-498b-8a63-e258524509a9


-✅ 完成済み

- レンダリング基盤
- ウィンドウ管理
- シェーダーシステム
- 基本図形描画 (三角形)
- キーボード、マウスインプット機能（将来はゲームパッドも視野に入れている）

---

## 🧱 構成（予定）

- `src/Core/` : エンジン本体（Application、Timer、Logなど）
- `src/Renderer/` : DirectX12ラッパー（Device、SwapChain、Commandなど）
- `src/Game/` : 実際のゲームロジックやオブジェクト
- `assets/` : テクスチャ・モデル・音声ファイル
- `include/` : ヘッダーの一部を分離する予定

---


## 🙌 このプロジェクトについて

高校生として、将来ゲームエンジニアになるために、基礎から丁寧に制作中です。

---


## このプロジェクトのダウンロード方法
🎮 **[最新版のダウンロードはこちら](https://github.com/HEROWL74/DX12GameEngine/releases/latest)**  

- DX12GameEngine.zip は、Asset と exe ファイルが入っているフォルダです。  
- DX12GameEngine-with-src.zip は、ソースファイルも一緒に入っているので、ただエンジンに触れたい方は DX12GameEngine.zip をおすすめします。  
- ダウンロード後、zip を解凍して exe をクリックすると、Windows が SmartScreen を表示します。  
  詳細情報 → 実行ボタン をクリックしてください。  

### SmartScreen の例
![SmartScreen の詳細画面](https://github.com/user-attachments/assets/58bb6fa6-d59e-43a3-b1cf-9ed16e860753)  
![SmartScreen の実行ボタン](https://github.com/user-attachments/assets/6ff98076-17f1-4f36-88d8-4dcbc1686140)


## 📧 作者について

- 名前：HEROWL
- X (旧Twitter)：[HRAKProgrammer](https://x.com/HRAKProgrammer)
- 開発環境：Visual Studio 2022 / Windows 11 / C++20&23
- 興味分野：ゲームエンジン開発、ゲーム開発、バスケットボール、水上オートバイ運転
