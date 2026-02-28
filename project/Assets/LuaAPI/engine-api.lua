---@meta
---@diagnostic disable

-- =========================
-- 基本型
-- =========================

---@class GameObject
---@field name string
---@field active boolean

-- =========================
-- コールバック型定義
-- =========================

---@alias OnStartFunc fun(obj: GameObject)
---@alias OnUpdateFunc fun(obj: GameObject, dt: number)
