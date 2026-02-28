#pragma once

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <vector>

struct SColumnNodeData {
    WP<Layout::ITarget> target;
};

struct SColumnData {
    std::vector<SP<SColumnNodeData>> nodes;
};

class CColumnsAlgorithm : public Layout::ITiledAlgorithm {
  public:
    CColumnsAlgorithm()          = default;
    virtual ~CColumnsAlgorithm() = default;

    void                             newTarget(SP<Layout::ITarget> target) override;
    void                             movedTarget(SP<Layout::ITarget> target, std::optional<Vector2D> focalPoint = std::nullopt) override;
    void                             removeTarget(SP<Layout::ITarget> target) override;
    void                             resizeTarget(const Vector2D& delta, SP<Layout::ITarget> target, Layout::eRectCorner corner = Layout::CORNER_NONE) override;
    void                             recalculate() override;
    void                             swapTargets(SP<Layout::ITarget> a, SP<Layout::ITarget> b) override;
    void                             moveTargetInDirection(SP<Layout::ITarget> t, Math::eDirection dir, bool silent) override;
    SP<Layout::ITarget>              getNextCandidate(SP<Layout::ITarget> old) override;
    std::expected<void, std::string> layoutMsg(const std::string_view& sv) override;
    std::optional<Vector2D>          predictSizeForNewTarget() override;

  private:
    std::vector<SP<SColumnData>> m_columns;

    void calculateWorkspace();
    void pruneEmptyColumns();

    // Find which column/node a target lives in
    struct STargetLocation {
        int colIdx  = -1;
        int nodeIdx = -1;
    };
    STargetLocation findTarget(SP<Layout::ITarget> target);

    int  getMaxColumns();
    bool isSpawnRight();
};
