#ifndef MODELER_COMPONENT_H
#define MODELER_COMPONENT_H

typedef struct offset_t {
	float x;
	float y;
} Offset;

typedef struct extent_t {
	float width;
	float height;
} Extent;

typedef struct normalized_rectangle_t {
        Offset offset;
	Extent extent;
} NormalizedRectangle;

#endif /* MODELER_COMPONENT_H */
