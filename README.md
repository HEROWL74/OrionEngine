# OrionEngine ==> Game Engine Prototype

[![Build OrionEngine](https://github.com/HEROWL74/OrionEngine/actions/workflows/bulid.yml/badge.svg?branch=develop)](https://github.com/HEROWL74/OrionEngine/actions/workflows/bulid.yml)

このプロジェクトは、高校生が独学で開発している**GameEngine**のプロトタイプです。  
C++ と DirectX12 を使って、エンジン内部の仕組みを一から学びながら構築しています。
Microsoftのリファレンスページや、サンプルコードを見ながら勉強しながらコーディングしています。
---

## 🎯 目的とビジョン

- ゲームエンジンの構造（Scene, Entity, Component, Rendererなど）を理解する
- UnityやUnreal Engineのような再利用性の高いフレームワークを目指す
- 将来的には **教育用エンジン**として使えるようにすることを目指す

---



## ✨ 現在の進捗 OrionEngine 第一作ミニゲーム完成
https://github.com/HEROWL74/OrionEngine/releases/tag/GameVol.1

### エディタ画像
![image](https://github.com/user-attachments/assets/66efdfdc-a671-4e18-ba63-b5c6466276ba)
### 作成したミニゲームスクリーンショット
![image](https://github.com/user-attachments/assets/e82ae3d1-695b-499d-b06b-5fa8d6067d2e)

-✅ 完成済み

- レンダリング基盤
- ウィンドウ管理
- シェーダーシステム
- 基本図形描画 (三角形)
- キーボード、マウスインプット機能（将来はゲームパッドも視野に入れている）

---

## 🧱 構成（予定）

- `src/engine/Core` : エンジン本体（Application、GameObject、Windowなど）
- `src/engine/Graphics/` : DirectX12ラッパー（Device、SwapChain、Commandなど）
- `src/engine/` : 実際のゲームロジックやオブジェクト
- `engine-assets` : エンジン内蔵のシェーダーや画像格納フォルダ
- `assets/` : 実際にユーザーが編集するフォルダ

---


## 🙌 このプロジェクトについて

高校生として、将来ゲームエンジニアになるために、基礎から丁寧に制作中です。

## 📧 作者について

- 名前：HEROWL
- X (旧Twitter)：[HRAKProgrammer](https://x.com/HRAKProgrammer)
- 開発環境：Visual Studio 2022 / Windows 11 / C++20&23
- 興味分野：ゲームエンジン開発、ゲーム開発、バスケットボール、水上オートバイ運転
