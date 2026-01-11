#pragma once

#include <Qt>

namespace SankeyRoles {

enum Role {
    SourceRole = Qt::UserRole + 1,
    TargetRole,
    ValueRole,
    LabelRole, // optional, for later
    ColorRole // optional
};

}
