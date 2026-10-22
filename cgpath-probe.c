/* What the path API answers, printed so that two implementations can be
 * compared line for line.
 *
 * Built against the real CoreGraphics headers, so the declarations are the
 * SDK's rather than a hand-written copy.
 *
 *     cc -arch x86_64 -fblocks -framework CoreGraphics -o cgpath_probe
 * cgpath-probe.c
 *     ./cgpath_probe > expected-cgpath.txt
 *     rosemary run --emulated ./cgpath_probe
 *
 * The two must print the same thing.
 */

#include <CoreGraphics/CoreGraphics.h>
#include <stdbool.h>
#include <stdio.h>

/* Spelled out rather than `M_PI`, which strict C modes hide. */
static const double pi = 3.14159265358979323846;

static void number(double value) {
  /* `-0.0` and `0.0` are the same coordinate, and a diff that says otherwise
   * is noise rather than a difference. */
  printf("%.6f", value + 0.0);
}

static void point(const char *label, CGPoint p) {
  printf("  %s ", label);
  number(p.x);
  putchar(' ');
  number(p.y);
  putchar('\n');
}

static void rect(const char *label, CGRect r) {
  printf("  %s ", label);
  number(r.origin.x);
  putchar(' ');
  number(r.origin.y);
  putchar(' ');
  number(r.size.width);
  putchar(' ');
  number(r.size.height);
  putchar('\n');
}

static void boolean(const char *label, bool value) {
  printf("  %s %d\n", label, value ? 1 : 0);
}

/* One element of a path, as `CGPathApply` hands it over: a move and a line
 * carry one point, a quad two, a curve three, and a close none. */
static void print_element(void *info, const CGPathElement *element) {
  (void)info;
  switch (element->type) {
  case kCGPathElementMoveToPoint:
    printf("    move ");
    number(element->points[0].x);
    putchar(' ');
    number(element->points[0].y);
    putchar('\n');
    break;
  case kCGPathElementAddLineToPoint:
    printf("    line ");
    number(element->points[0].x);
    putchar(' ');
    number(element->points[0].y);
    putchar('\n');
    break;
  case kCGPathElementAddQuadCurveToPoint:
    printf("    quad ");
    number(element->points[0].x);
    putchar(' ');
    number(element->points[0].y);
    putchar(' ');
    number(element->points[1].x);
    putchar(' ');
    number(element->points[1].y);
    putchar('\n');
    break;
  case kCGPathElementAddCurveToPoint:
    printf("    curve ");
    number(element->points[0].x);
    putchar(' ');
    number(element->points[0].y);
    putchar(' ');
    number(element->points[1].x);
    putchar(' ');
    number(element->points[1].y);
    putchar(' ');
    number(element->points[2].x);
    putchar(' ');
    number(element->points[2].y);
    putchar('\n');
    break;
  case kCGPathElementCloseSubpath:
    printf("    close\n");
    break;
  }
}

/* Everything the introspection functions say about one path. The rectangle
 * handed to `CGPathIsRect` is zeroed first so that a false answer still prints
 * the same four numbers on both implementations. */
static void show(const char *label, CGPathRef path) {
  CGRect is_rect = {{0.0, 0.0}, {0.0, 0.0}};

  printf("path %s\n", label);
  printf("  elements\n");
  CGPathApply(path, NULL, print_element);
  boolean("empty", CGPathIsEmpty(path));
  point("current", CGPathGetCurrentPoint(path));
  rect("bbox", CGPathGetBoundingBox(path));
  rect("path-bbox", CGPathGetPathBoundingBox(path));
  boolean("is-rect", CGPathIsRect(path, &is_rect));
  rect("rect", is_rect);
  /* The same point under both fill rules: a path that crosses itself is where
   * the two disagree. */
  boolean("contains-eo",
          CGPathContainsPoint(path, NULL, (CGPoint){5.0, 5.0}, true));
  boolean("contains-nonzero",
          CGPathContainsPoint(path, NULL, (CGPoint){5.0, 5.0}, false));
}

int main(void) {
  CGAffineTransform identity = CGAffineTransformIdentity;
  CGAffineTransform scale = {2.0, 0.0, 0.0, 2.0, 0.0, 0.0};

  printf("case create-mutable\n");
  CGMutablePathRef base = CGPathCreateMutable();
  show("empty-mutable", base);

  printf("case build\n");
  CGPathMoveToPoint(base, &identity, 0.0, 0.0);
  CGPathAddLineToPoint(base, NULL, 10.0, 0.0);
  CGPathAddQuadCurveToPoint(base, NULL, 15.0, 5.0, 10.0, 10.0);
  CGPathAddCurveToPoint(base, NULL, 8.0, 12.0, 2.0, 12.0, 0.0, 10.0);
  CGPathCloseSubpath(base);
  show("base", base);

  printf("case add-rect\n");
  CGMutablePathRef p = CGPathCreateMutable();
  CGPathAddRect(p, NULL, (CGRect){{1.0, 2.0}, {3.0, 4.0}});
  show("add-rect", p);
  CGPathRelease(p);

  printf("case add-rects\n");
  p = CGPathCreateMutable();
  CGRect rects[2] = {{{0.0, 0.0}, {2.0, 2.0}}, {{4.0, 4.0}, {2.0, 2.0}}};
  CGPathAddRects(p, NULL, rects, 2);
  show("add-rects", p);
  CGPathRelease(p);

  printf("case add-lines\n");
  p = CGPathCreateMutable();
  CGPoint lines[3] = {{0.0, 0.0}, {5.0, 5.0}, {10.0, 0.0}};
  CGPathAddLines(p, NULL, lines, 3);
  show("add-lines", p);
  CGPathRelease(p);

  printf("case add-ellipse\n");
  p = CGPathCreateMutable();
  CGPathAddEllipseInRect(p, NULL, (CGRect){{0.0, 0.0}, {10.0, 6.0}});
  show("add-ellipse", p);
  CGPathRelease(p);

  printf("case add-relative-arc\n");
  p = CGPathCreateMutable();
  CGPathAddRelativeArc(p, NULL, 5.0, 5.0, 4.0, 0.0, pi / 2.0);
  show("add-relative-arc", p);
  CGPathRelease(p);

  printf("case add-arc-clockwise\n");
  p = CGPathCreateMutable();
  CGPathAddArc(p, NULL, 5.0, 5.0, 4.0, 0.0, pi, true);
  show("add-arc-clockwise", p);
  CGPathRelease(p);

  printf("case add-arc-counterclockwise\n");
  p = CGPathCreateMutable();
  CGPathAddArc(p, NULL, 5.0, 5.0, 4.0, 0.0, pi, false);
  show("add-arc-counterclockwise", p);
  CGPathRelease(p);

  printf("case add-arc-to-point\n");
  p = CGPathCreateMutable();
  CGPathMoveToPoint(p, NULL, 0.0, 0.0);
  CGPathAddArcToPoint(p, NULL, 5.0, 0.0, 5.0, 5.0, 2.0);
  show("add-arc-to-point", p);
  CGPathRelease(p);

  printf("case add-rounded-rect\n");
  p = CGPathCreateMutable();
  CGPathAddRoundedRect(p, NULL, (CGRect){{0.0, 0.0}, {10.0, 8.0}}, 2.0, 2.0);
  show("add-rounded-rect", p);
  CGPathRelease(p);

  printf("case add-path\n");
  p = CGPathCreateMutable();
  CGPathAddPath(p, &scale, base);
  show("add-path", p);
  CGPathRelease(p);

  printf("case create-with-rect\n");
  CGPathRef with_rect =
      CGPathCreateWithRect((CGRect){{0.0, 0.0}, {10.0, 10.0}}, NULL);
  show("create-with-rect", with_rect);
  CGPathRelease(with_rect);

  printf("case create-with-ellipse\n");
  CGPathRef with_ellipse =
      CGPathCreateWithEllipseInRect((CGRect){{0.0, 0.0}, {10.0, 6.0}}, NULL);
  show("create-with-ellipse", with_ellipse);
  CGPathRelease(with_ellipse);

  printf("case create-with-rounded-rect\n");
  CGPathRef with_rounded = CGPathCreateWithRoundedRect(
      (CGRect){{0.0, 0.0}, {10.0, 8.0}}, 2.0, 2.0, NULL);
  show("create-with-rounded-rect", with_rounded);
  CGPathRelease(with_rounded);

  printf("case copy\n");
  CGPathRef copy = CGPathCreateCopy(base);
  show("copy", copy);
  CGPathRelease(copy);

  printf("case mutable-copy\n");
  CGMutablePathRef mutable_copy = CGPathCreateMutableCopy(base);
  show("mutable-copy", mutable_copy);
  CGPathRelease(mutable_copy);

  printf("case transformed-copy\n");
  CGPathRef transformed = CGPathCreateCopyByTransformingPath(base, &scale);
  show("transformed-copy", transformed);
  CGPathRelease(transformed);

  printf("case mutable-transformed-copy\n");
  CGMutablePathRef mutable_transformed =
      CGPathCreateMutableCopyByTransformingPath(base, &scale);
  show("mutable-transformed-copy", mutable_transformed);
  CGPathRelease(mutable_transformed);

  printf("case dashed-copy\n");
  CGFloat dashes[2] = {2.0, 1.0};
  CGPathRef dashed = CGPathCreateCopyByDashingPath(base, NULL, 0.5, dashes, 2);
  show("dashed-copy", dashed);
  CGPathRelease(dashed);

  printf("case stroked-copy\n");
  CGPathRef stroked = CGPathCreateCopyByStrokingPath(
      base, NULL, 1.0, kCGLineCapRound, kCGLineJoinRound, 10.0);
  show("stroked-copy", stroked);
  CGPathRelease(stroked);

  printf("case flattened-copy\n");
  CGPathRef flattened = CGPathCreateCopyByFlattening(base, 0.5);
  show("flattened-copy", flattened);
  CGPathRelease(flattened);

  printf("case normalized-copy\n");
  CGPathRef normalized = CGPathCreateCopyByNormalizing(base, false);
  show("normalized-copy", normalized);
  CGPathRelease(normalized);

  printf("case boolean\n");
  CGPathRef rect_a =
      CGPathCreateWithRect((CGRect){{0.0, 0.0}, {10.0, 10.0}}, NULL);
  CGPathRef rect_b =
      CGPathCreateWithRect((CGRect){{5.0, 5.0}, {10.0, 10.0}}, NULL);

  CGPathRef intersect =
      CGPathCreateCopyByIntersectingPath(rect_a, rect_b, false);
  show("intersect", intersect);
  CGPathRelease(intersect);

  CGPathRef unioned = CGPathCreateCopyByUnioningPath(rect_a, rect_b, false);
  show("union", unioned);
  CGPathRelease(unioned);

  CGPathRef subtracted =
      CGPathCreateCopyBySubtractingPath(rect_a, rect_b, false);
  show("subtract", subtracted);
  CGPathRelease(subtracted);

  CGPathRef symmetric =
      CGPathCreateCopyBySymmetricDifferenceOfPath(rect_a, rect_b, false);
  show("symmetric-difference", symmetric);
  CGPathRelease(symmetric);

  printf("case interpolate\n");
  /* Two rectangles built the same way, so the element sequences line up. */
  CGPathRef interpolated =
      CGPathCreateCopyByLinearlyInterpolatingPaths(rect_a, rect_b, 0.5);
  show("interpolated", interpolated);
  CGPathRelease(interpolated);

  printf("case introspection\n");
  point("current-base", CGPathGetCurrentPoint(base));
  boolean("empty-base", CGPathIsEmpty(base));

  CGMutablePathRef empty = CGPathCreateMutable();
  boolean("empty-new", CGPathIsEmpty(empty));
  CGPathRelease(empty);

  CGRect is_rect = {{0.0, 0.0}, {0.0, 0.0}};
  boolean("is-rect-base", CGPathIsRect(base, &is_rect));
  rect("is-rect-base-rect", is_rect);

  CGPathRef copy2 = CGPathCreateCopy(base);
  CGPathRef other =
      CGPathCreateWithRect((CGRect){{0.0, 0.0}, {10.0, 10.0}}, NULL);
  boolean("equal-self", CGPathEqualToPath(base, base));
  boolean("equal-copy", CGPathEqualToPath(base, copy2));
  boolean("equal-other", CGPathEqualToPath(base, other));
  CGPathRelease(copy2);
  CGPathRelease(other);

  printf("type-id %lu\n", (unsigned long)CGPathGetTypeID());

#ifdef __BLOCKS__
  printf("case apply-with-block\n");
  CGPathApplyWithBlock(base, ^(const CGPathElement *element) {
    print_element(NULL, element);
  });
#endif

  printf("case retain\n");
  CGPathRef retained = CGPathRetain(base);
  boolean("retained-not-null", retained != NULL);
  CGPathRelease(retained);

  CGPathRelease(rect_a);
  CGPathRelease(rect_b);
  CGPathRelease(base);

  return 0;
}
