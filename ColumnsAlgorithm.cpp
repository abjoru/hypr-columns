#include "ColumnsAlgorithm.hpp"
#include "globals.hpp"
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/space/Space.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <algorithm>

// ── Helpers ──────────────────────────────────────────────────────────────

int CColumnsAlgorithm::getMaxColumns() {
    static auto PMAXCOLS = CConfigValue<Hyprlang::INT>("plugin:hypr-columns:max_columns");
    return std::max((Hyprlang::INT)1, *PMAXCOLS);
}

bool CColumnsAlgorithm::isSpawnRight() {
    static auto PDIR = CConfigValue<std::string>("plugin:hypr-columns:spawn_direction");
    return *PDIR != "left";
}

CColumnsAlgorithm::STargetLocation CColumnsAlgorithm::findTarget(SP<Layout::ITarget> target) {
    for (int c = 0; c < (int)m_columns.size(); c++) {
        auto& nodes = m_columns[c]->nodes;
        for (int n = 0; n < (int)nodes.size(); n++) {
            if (nodes[n]->target.lock() == target)
                return {c, n};
        }
    }
    return {-1, -1};
}

void CColumnsAlgorithm::pruneEmptyColumns() {
    std::erase_if(m_columns, [](const SP<SColumnData>& col) {
        std::erase_if(col->nodes, [](const SP<SColumnNodeData>& n) { return n->target.expired(); });
        return col->nodes.empty();
    });
}

// ── Core Layout ──────────────────────────────────────────────────────────

void CColumnsAlgorithm::calculateWorkspace() {
    pruneEmptyColumns();

    if (m_columns.empty())
        return;

    // redistribute: if total windows <= max_columns, one window per column
    int totalWindows = 0;
    for (auto& col : m_columns)
        totalWindows += (int)col->nodes.size();

    if (totalWindows <= getMaxColumns()) {
        std::vector<SP<SColumnData>> newCols;
        for (auto& col : m_columns) {
            for (auto& node : col->nodes) {
                auto nc = makeShared<SColumnData>();
                nc->nodes.push_back(node);
                newCols.push_back(nc);
            }
        }
        m_columns = std::move(newCols);
    }

    auto parent = m_parent.lock();
    if (!parent)
        return;

    auto space = parent->space();
    if (!space)
        return;

    const auto& workArea = space->workArea();
    const int   numCols  = (int)m_columns.size();
    const double colWidth = workArea.w / numCols;

    for (int c = 0; c < numCols; c++) {
        auto& nodes    = m_columns[c]->nodes;
        const int numWins = (int)nodes.size();
        if (numWins == 0)
            continue;

        const double winHeight = workArea.h / numWins;

        for (int n = 0; n < numWins; n++) {
            auto target = nodes[n]->target.lock();
            if (!target)
                continue;

            target->setPositionGlobal(CBox(
                workArea.x + c * colWidth,
                workArea.y + n * winHeight,
                colWidth,
                winHeight
            ));
        }
    }
}

// ── Virtual Methods ──────────────────────────────────────────────────────

void CColumnsAlgorithm::newTarget(SP<Layout::ITarget> target) {
    auto node = makeShared<SColumnNodeData>();
    node->target = target;

    const int  maxCols = getMaxColumns();
    const bool right   = isSpawnRight();

    if ((int)m_columns.size() < maxCols) {
        auto col = makeShared<SColumnData>();
        col->nodes.push_back(node);

        if (right)
            m_columns.push_back(col);
        else
            m_columns.insert(m_columns.begin(), col);
    } else {
        // find least populated column
        SP<SColumnData>* least = &(right ? m_columns.back() : m_columns.front());
        for (auto& col : m_columns) {
            if (col->nodes.size() < (*least)->nodes.size())
                least = &col;
        }
        (*least)->nodes.push_back(node);
    }

    calculateWorkspace();
}

void CColumnsAlgorithm::movedTarget(SP<Layout::ITarget> target, std::optional<Vector2D> focalPoint) {
    if (focalPoint.has_value() && !m_columns.empty()) {
        auto parent = m_parent.lock();
        if (parent) {
            auto space = parent->space();
            if (space) {
                const auto& workArea = space->workArea();
                const int   numCols  = (int)m_columns.size();
                const double colWidth = workArea.w / numCols;
                int colIdx = std::clamp((int)((focalPoint->x - workArea.x) / colWidth), 0, numCols - 1);

                auto node = makeShared<SColumnNodeData>();
                node->target = target;
                m_columns[colIdx]->nodes.push_back(node);
                calculateWorkspace();
                return;
            }
        }
    }

    newTarget(target);
}

void CColumnsAlgorithm::removeTarget(SP<Layout::ITarget> target) {
    auto [c, n] = findTarget(target);
    if (c < 0)
        return;

    m_columns[c]->nodes.erase(m_columns[c]->nodes.begin() + n);
    pruneEmptyColumns();
    calculateWorkspace();
}

void CColumnsAlgorithm::resizeTarget(const Vector2D&, SP<Layout::ITarget>, Layout::eRectCorner) {
    // no-op: equal-width invariant
}

void CColumnsAlgorithm::recalculate() {
    calculateWorkspace();
}

void CColumnsAlgorithm::swapTargets(SP<Layout::ITarget> a, SP<Layout::ITarget> b) {
    auto locA = findTarget(a);
    auto locB = findTarget(b);
    if (locA.colIdx < 0 || locB.colIdx < 0)
        return;

    std::swap(m_columns[locA.colIdx]->nodes[locA.nodeIdx]->target,
              m_columns[locB.colIdx]->nodes[locB.nodeIdx]->target);
    calculateWorkspace();
}

void CColumnsAlgorithm::moveTargetInDirection(SP<Layout::ITarget> target, Math::eDirection dir, bool silent) {
    auto [c, n] = findTarget(target);
    if (c < 0)
        return;

    switch (dir) {
        case Math::DIRECTION_LEFT: {
            if (c == 0) {
                if ((int)m_columns.size() < getMaxColumns()) {
                    m_columns[c]->nodes.erase(m_columns[c]->nodes.begin() + n);
                    auto col  = makeShared<SColumnData>();
                    auto node = makeShared<SColumnNodeData>();
                    node->target = target;
                    col->nodes.push_back(node);
                    m_columns.insert(m_columns.begin(), col);
                }
            } else if ((int)m_columns[c]->nodes.size() == 1) {
                // sole window in column — swap columns
                std::swap(m_columns[c], m_columns[c - 1]);
            } else {
                m_columns[c]->nodes.erase(m_columns[c]->nodes.begin() + n);
                auto node = makeShared<SColumnNodeData>();
                node->target = target;
                m_columns[c - 1]->nodes.push_back(node);
            }
            break;
        }
        case Math::DIRECTION_RIGHT: {
            if (c == (int)m_columns.size() - 1) {
                if ((int)m_columns.size() < getMaxColumns()) {
                    m_columns[c]->nodes.erase(m_columns[c]->nodes.begin() + n);
                    auto col  = makeShared<SColumnData>();
                    auto node = makeShared<SColumnNodeData>();
                    node->target = target;
                    col->nodes.push_back(node);
                    m_columns.push_back(col);
                }
            } else if ((int)m_columns[c]->nodes.size() == 1) {
                std::swap(m_columns[c], m_columns[c + 1]);
            } else {
                m_columns[c]->nodes.erase(m_columns[c]->nodes.begin() + n);
                auto node = makeShared<SColumnNodeData>();
                node->target = target;
                m_columns[c + 1]->nodes.push_back(node);
            }
            break;
        }
        case Math::DIRECTION_UP: {
            if (n > 0)
                std::swap(m_columns[c]->nodes[n], m_columns[c]->nodes[n - 1]);
            break;
        }
        case Math::DIRECTION_DOWN: {
            if (n < (int)m_columns[c]->nodes.size() - 1)
                std::swap(m_columns[c]->nodes[n], m_columns[c]->nodes[n + 1]);
            break;
        }
        default: break;
    }

    pruneEmptyColumns();
    calculateWorkspace();
}

SP<Layout::ITarget> CColumnsAlgorithm::getNextCandidate(SP<Layout::ITarget> old) {
    auto [c, n] = findTarget(old);

    // target already removed? scan for any remaining
    if (c < 0) {
        for (auto& col : m_columns)
            for (auto& node : col->nodes)
                if (!node->target.expired() && node->target.lock() != old)
                    return node->target.lock();
        return nullptr;
    }

    auto& nodes = m_columns[c]->nodes;

    // try neighbor in same column
    if ((int)nodes.size() > 1) {
        int next = (n > 0) ? n - 1 : n + 1;
        if (next < (int)nodes.size())
            return nodes[next]->target.lock();
    }

    // try adjacent column
    if (c + 1 < (int)m_columns.size() && !m_columns[c + 1]->nodes.empty())
        return m_columns[c + 1]->nodes[0]->target.lock();
    if (c - 1 >= 0 && !m_columns[c - 1]->nodes.empty())
        return m_columns[c - 1]->nodes[0]->target.lock();

    return nullptr;
}

// ── Layout Messages ──────────────────────────────────────────────────────

std::expected<void, std::string> CColumnsAlgorithm::layoutMsg(const std::string_view& sv) {
    auto parent = m_parent.lock();
    if (!parent)
        return std::unexpected("no parent");

    auto space = parent->space();
    if (!space)
        return std::unexpected("no space");

    // parse "command arg"
    auto spacePos = sv.find(' ');
    auto cmd      = sv.substr(0, spacePos);
    int  arg      = 0;
    if (spacePos != std::string_view::npos) {
        auto argStr = sv.substr(spacePos + 1);
        arg = (argStr == "+1" || argStr == "1") ? 1 : (argStr == "-1") ? -1 : 0;
    }

    if (arg == 0)
        return std::unexpected("invalid arg, use +1 or -1");

    // find currently focused target
    SP<Layout::ITarget> focused;

    auto focusedWindow = Desktop::focusState()->window();
    if (focusedWindow) {
        auto& targets = space->targets();
        for (auto& wt : targets) {
            auto t = wt.lock();
            if (!t)
                continue;
            if (t->window() == focusedWindow) {
                focused = t;
                break;
            }
        }
    }

    if (!focused)
        return std::unexpected("no focused window");

    auto loc       = findTarget(focused);
    int  focusedCol = loc.colIdx;
    if (focusedCol < 0)
        return std::unexpected("focused window not in columns");

    int targetCol = focusedCol + arg;

    if (cmd == "focuscolumn") {
        if (targetCol < 0 || targetCol >= (int)m_columns.size())
            return {};

        auto& nodes = m_columns[targetCol]->nodes;
        if (!nodes.empty()) {
            auto t = nodes[0]->target.lock();
            if (t) {
                auto w = t->window();
                if (w)
                    Desktop::focusState()->fullWindowFocus(w, Desktop::FOCUS_REASON_KEYBIND);
            }
        }
        return {};
    }

    if (cmd == "movetocolumn") {
        if (targetCol < 0 || targetCol >= (int)m_columns.size())
            return {};

        m_columns[focusedCol]->nodes.erase(m_columns[focusedCol]->nodes.begin() + loc.nodeIdx);
        auto node = makeShared<SColumnNodeData>();
        node->target = focused;
        m_columns[targetCol]->nodes.push_back(node);
        pruneEmptyColumns();
        calculateWorkspace();
        return {};
    }

    if (cmd == "swapcolumn") {
        if (targetCol < 0 || targetCol >= (int)m_columns.size())
            return {};

        std::swap(m_columns[focusedCol], m_columns[targetCol]);
        calculateWorkspace();
        return {};
    }

    return std::unexpected("unknown command: " + std::string(cmd));
}

// ── Predict Size ─────────────────────────────────────────────────────────

std::optional<Vector2D> CColumnsAlgorithm::predictSizeForNewTarget() {
    auto parent = m_parent.lock();
    if (!parent)
        return std::nullopt;

    auto space = parent->space();
    if (!space)
        return std::nullopt;

    const auto& workArea = space->workArea();
    const int   maxCols  = getMaxColumns();
    const int   numCols  = std::min((int)m_columns.size() + 1, maxCols);

    return Vector2D(workArea.w / numCols, workArea.h);
}
