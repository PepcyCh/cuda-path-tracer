#pragma once

#include "accel.cuh"

namespace kernel {

void BuildAccel(AccelNode *nodes, Bbox *bboxes, Bbox merged_bbox, uint32_t num_primitives);

}
