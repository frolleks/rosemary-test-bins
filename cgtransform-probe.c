/* What the affine-transform API answers, printed so that two implementations
 * can be compared line for line.
 *
 * Freestanding on purpose: the declarations below are
 * <CoreGraphics/CGGeometry.h>'s, spelled out so the program builds without a
 * macOS SDK. See build-guest.sh for the cross-compile recipe.
 *
 *     cc -arch x86_64 -framework CoreGraphics -o cgtransform_probe
 * cgtransform_probe.c
 *     ./cgtransform_probe > expected.txt
 *     rosemary run --emulated ./cgtransform_probe
 *
 * The two must print the same thing.
 */

typedef _Bool cg_bool;
typedef double CGFloat;

typedef struct CGPoint {
  CGFloat x, y;
} CGPoint;
typedef struct CGSize {
  CGFloat width, height;
} CGSize;
typedef struct CGRect {
  CGPoint origin;
  CGSize size;
} CGRect;
typedef struct CGAffineTransform {
  CGFloat a, b, c, d, tx, ty;
} CGAffineTransform;

extern int printf(const char *format, ...);
extern int putchar(int c);

extern const CGAffineTransform CGAffineTransformIdentity;

extern CGAffineTransform CGAffineTransformMake(CGFloat a, CGFloat b, CGFloat c,
                                               CGFloat d, CGFloat tx,
                                               CGFloat ty);
extern CGAffineTransform CGAffineTransformMakeScale(CGFloat sx, CGFloat sy);
extern CGAffineTransform CGAffineTransformMakeTranslation(CGFloat tx,
                                                          CGFloat ty);
extern CGAffineTransform CGAffineTransformScale(CGAffineTransform t, CGFloat sx,
                                                CGFloat sy);
extern CGAffineTransform CGAffineTransformTranslate(CGAffineTransform t,
                                                    CGFloat tx, CGFloat ty);
extern CGAffineTransform CGAffineTransformConcat(CGAffineTransform t1,
                                                 CGAffineTransform t2);
extern CGAffineTransform CGAffineTransformInvert(CGAffineTransform t);
extern cg_bool CGAffineTransformIsIdentity(CGAffineTransform t);
extern CGRect CGRectApplyAffineTransform(CGRect rect, CGAffineTransform t);

static void number(double value) {
  /* `-0.0` and `0.0` are the same transform, and a diff that says otherwise
   * is noise rather than a difference. */
  printf("%.6f", value + 0.0);
}

static void transform(const char *label, CGAffineTransform t) {
  printf("  %s ", label);
  number(t.a);
  putchar(' ');
  number(t.b);
  putchar(' ');
  number(t.c);
  putchar(' ');
  number(t.d);
  putchar(' ');
  number(t.tx);
  putchar(' ');
  number(t.ty);
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

static void identity(const char *label, CGAffineTransform t) {
  printf("  %s %d\n", label, CGAffineTransformIsIdentity(t) ? 1 : 0);
}

/* A point, through the implementation: a zero-sized rectangle transforms to
 * the point, which is `CGPointApplyAffineTransform` without the function this
 * tree does not implement. */
static void point(const char *label, CGPoint p, CGAffineTransform t) {
  CGRect box = {{p.x, p.y}, {0.0, 0.0}};
  CGRect out = CGRectApplyAffineTransform(box, t);
  printf("  %s ", label);
  number(out.origin.x);
  putchar(' ');
  number(out.origin.y);
  putchar('\n');
}

int main(void) {
  CGRect box = {{0.0, 0.0}, {10.0, 20.0}};

  printf("case identity\n");
  transform("value", CGAffineTransformIdentity);
  identity("is-identity", CGAffineTransformIdentity);

  printf("case make\n");
  CGAffineTransform t = CGAffineTransformMake(2.0, 3.0, 4.0, 5.0, 6.0, 7.0);
  transform("value", t);
  identity("is-identity", t);

  printf("case make-scale\n");
  transform("value", CGAffineTransformMakeScale(2.0, 3.0));

  printf("case make-translation\n");
  transform("value", CGAffineTransformMakeTranslation(5.0, -7.0));

  printf("case scale\n");
  /* `Scale` and `Translate` apply *before* the transform they are given:
   * the point is scaled, then `t` moves it. */
  transform("value", CGAffineTransformScale(t, 2.0, 3.0));

  printf("case translate\n");
  transform("value", CGAffineTransformTranslate(t, 5.0, -7.0));

  printf("case concat\n");
  CGAffineTransform scale = CGAffineTransformMakeScale(2.0, 2.0);
  CGAffineTransform move = CGAffineTransformMakeTranslation(10.0, 0.0);
  transform("scale-then-move", CGAffineTransformConcat(scale, move));
  transform("move-then-scale", CGAffineTransformConcat(move, scale));
  /* The order is the whole point: the same two transforms, different
   * answers. */
  rect("scale-then-move-rect",
       CGRectApplyAffineTransform(box, CGAffineTransformConcat(scale, move)));
  rect("move-then-scale-rect",
       CGRectApplyAffineTransform(box, CGAffineTransformConcat(move, scale)));

  printf("case invert\n");
  CGAffineTransform inverse = CGAffineTransformInvert(t);
  transform("value", inverse);
  identity("round-trip", CGAffineTransformConcat(t, inverse));
  /* A singular transform is returned unchanged, as Quartz documents. */
  transform("singular",
            CGAffineTransformInvert(CGAffineTransformMakeScale(0.0, 1.0)));

  printf("case apply-rect\n");
  rect("identity", CGRectApplyAffineTransform(box, CGAffineTransformIdentity));
  rect("scaled",
       CGRectApplyAffineTransform(box, CGAffineTransformMakeScale(2.0, 3.0)));
  rect("translated", CGRectApplyAffineTransform(
                         box, CGAffineTransformMakeTranslation(5.0, -7.0)));
  /* A quarter turn, written out because `CGAffineTransformMakeRotation` is
   * not implemented in this tree. */
  CGAffineTransform quarter = {0.0, 1.0, -1.0, 0.0, 0.0, 0.0};
  rect("rotated", CGRectApplyAffineTransform(box, quarter));

  printf("case apply-point\n");
  point("origin", (CGPoint){0.0, 0.0}, t);
  point("unit", (CGPoint){1.0, 0.0}, t);

  return 0;
}
