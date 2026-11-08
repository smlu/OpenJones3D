#include "JonesReticle.h"

#include <rdroid/types.h>
#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/World/sithWorld.h>

#include <std/types.h>
#include <std/General/stdConfig.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include <math.h>

// Configuration keys
#define JONESRETICLE_CFG_ENABLED             "ui.hud.reticle.enabled"
#define JONESRETICLE_CFG_RADIUS              "ui.hud.reticle.radius"
#define JONESRETICLE_CFG_ARROWSIZE           "ui.hud.reticle.arrowSize"
#define JONESRETICLE_CFG_ARROWWIDTHSCALE     "ui.hud.reticle.arrowWidthScale"
#define JONESRETICLE_CFG_LOCKSCALE           "ui.hud.reticle.lockScale"               // lock in animation start scale
#define JONESRETICLE_CFG_LOCKDURATION        "ui.hud.reticle.lockAnimationDuration"   // lock in animation duration
#define JONESRETICLE_CFG_UNLOCKSCALE         "ui.hud.reticle.unlockScale"             // unlock animation end scale
#define JONESRETICLE_CFG_UNLOCKDURATION      "ui.hud.reticle.unlockAnimationDuration" // unlock animation duration
#define JONESRETICLE_CFG_ROTATIONSPEED       "ui.hud.reticle.rotationSpeed"
#define JONESRETICLE_CFG_PULSEMAGNITUDE      "ui.hud.reticle.pulseMagnitude"
#define JONESRETICLE_CFG_COLOR               "ui.hud.reticle.color"                   // HTML / HEX color  in format #RRGGBBAA

// Distance scaling
#define JONESRETICLE_CFG_DISTANCESCALING  "ui.hud.reticle.distanceScaling"
#define JONESRETICLE_CFG_MINDISTANCE      "ui.hud.reticle.minDistance"
#define JONESRETICLE_CFG_MAXDISTANCE      "ui.hud.reticle.maxDistance"
#define JONESRETICLE_CFG_MINDISTANCESCALE "ui.hud.reticle.minDistanceScale"
#define JONESRETICLE_CFG_MAXDISTANCESCALE "ui.hud.reticle.maxDistanceScale"

// Default values
#define JONESRETICLE_DEFAULT_ENABLED          true
#define JONESRETICLE_DEFAULT_RADIUS           50.0f
#define JONESRETICLE_DEFAULT_ARROWSIZE        11.0f
#define JONESRETICLE_DEFAULT_ARROWWIDTHSCALE  2.5f
#define JONESRETICLE_DEFAULT_LOCKSCALE        2.5f
#define JONESRETICLE_DEFAULT_LOCKDURATION     0.3f   // 300ms lock-on animation
#define JONESRETICLE_DEFAULT_UNLOCKSCALE      1.5f
#define JONESRETICLE_DEFAULT_UNLOCKDURATION   0.2f   // 200ms unlock animation
#define JONESRETICLE_DEFAULT_ROTATIONSPEED    45.0f  // 45 degrees per second
#define JONESRETICLE_DEFAULT_PULSEMAGNITUDE   0.45f
#define JONESRETICLE_DEFAULT_COLOR            (rdVector4){ .red = 0.85f, .green = 0.80f, .blue = 0.20f, .alpha=0.65f }

#define JONESRETICLE_DEFAULT_MINDISTANCE      0.1f
#define JONESRETICLE_DEFAULT_MAXDISTANCE      2.18f
#define JONESRETICLE_DEFAULT_MINDISTANCESCALE 2.5f
#define JONESRETICLE_DEFAULT_MAXDISTANCESCALE 0.45f 


// Reticle state for animation
typedef enum eJonesReticleState
{
    JONESRETICLE_STATE_INACTIVE   = 0, // Not shown
    JONESRETICLE_STATE_LOCKING    = 1, // Scaling in animation
    JONESRETICLE_STATE_LOCKED     = 2, // Locked on target
    JONESRETICLE_STATE_UNLOCKING  = 3, // Scaling out animation
} JonesReticleState;

// Reticle configuration structure
typedef struct sJonesLockOnReticle
{
    bool bEnabled;

    // Target info
    const SithThing* pTarget;     // Current thing the player is aiming at
    rdVector3 targetPos;          // Adjusted target pos

    // Visual properties
    float baseRadius;             // Normal radius when fully locked
    float baseArrowSize;          // Normal arrow size
    float arrowWidthScale;        // Width scale for arrows

    // Animation state
    bool bPlaying;
    JonesReticleState state;
    float animTime;               // Current animation time (0.0-1.0)

    float lockStartScale;         // Starting scale for lock-in animation
    float lockDuration;           // How long lock-in animation takes (seconds)

    float unlockEndScale;         // Ending scale for unlock animation
    float unlockDuration;         // How long unlock animation takes (seconds)

    float pulseTime;              // Time for pulse animation
    float pulseMagnitude;         // Magnitude of pulse animation

    float rotationAngle;          // Current rotation angle in radians
    float rotationSpeed;          // Rotation speed (radians per second)

    // Color configuration
    rdVector4 color;

    // Distance scaling
    float minDistance;             // Distance at which reticle is largest
    float maxDistance;             // Distance at which reticle is smaller
    float minDistanceScale;        // Scale multiplier at min distance (e.g., 5.5 = 550% size)
    float maxDistanceScale;        // Scale multiplier at max distance (e.g., 0.5 =  50% size)

} JonesAimReticle;

// Screen projection
typedef struct sJonesReticleScreenPos
{
    float sx; // screen x
    float sy; // screen y
    bool bVisible; // Is target pos visible on screen?
} JonesReticleScreenPos;


bool JonesReticle_bOpen = false;
JonesAimReticle JonesReticle_reticle = { 0 };


// Add to stdColor.h or a utility header
static inline rdVector4 JonesReticle_ColorToVector4(tStdColor color)
{
    rdVector4 result;
    result.red   = STD_GETRED(color) / 255.0f;
    result.green = STD_GETGREEN(color) / 255.0f;
    result.blue  = STD_GETBLUE(color) / 255.0f;
    result.alpha = STD_GETALPHA(color) / 255.0f;
    return result;
}

static inline tStdColor JonesReticle_ColorFromVector4(const rdVector4* pVector)
{
    return STD_RGBA(
        (uint8_t)(pVector->red * 255.0f),
        (uint8_t)(pVector->green * 255.0f),
        (uint8_t)(pVector->blue * 255.0f),
        (uint8_t)(pVector->alpha * 255.0f)
    );
}

void JonesReticle_Open(void)
{
    if ( JonesReticle_bOpen )
    {
        return;
    }

    // Load base configuration
    JonesReticle_reticle.bEnabled = stdConfig_GetBool(JONESRETICLE_CFG_ENABLED, JONESRETICLE_DEFAULT_ENABLED);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_ENABLED) )
    {
        stdConfig_SetBool(JONESRETICLE_CFG_ENABLED, JONESRETICLE_DEFAULT_ENABLED);
    }

    JonesReticle_reticle.baseRadius = stdConfig_GetFloat(JONESRETICLE_CFG_RADIUS, JONESRETICLE_DEFAULT_RADIUS);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_RADIUS) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_RADIUS, JONESRETICLE_DEFAULT_RADIUS);
    }

    JonesReticle_reticle.baseArrowSize = stdConfig_GetFloat(JONESRETICLE_CFG_ARROWSIZE, JONESRETICLE_DEFAULT_ARROWSIZE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_ARROWSIZE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_ARROWSIZE, JONESRETICLE_DEFAULT_ARROWSIZE);
    }

    JonesReticle_reticle.arrowWidthScale = stdConfig_GetFloat(JONESRETICLE_CFG_ARROWWIDTHSCALE, JONESRETICLE_DEFAULT_ARROWWIDTHSCALE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_ARROWWIDTHSCALE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_ARROWWIDTHSCALE, JONESRETICLE_DEFAULT_ARROWWIDTHSCALE);
    }

    JonesReticle_reticle.lockStartScale = stdConfig_GetFloat(JONESRETICLE_CFG_LOCKSCALE, JONESRETICLE_DEFAULT_LOCKSCALE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_LOCKSCALE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_LOCKSCALE, JONESRETICLE_DEFAULT_LOCKSCALE);
    }

    JonesReticle_reticle.lockDuration = stdConfig_GetFloat(JONESRETICLE_CFG_LOCKDURATION, JONESRETICLE_DEFAULT_LOCKDURATION);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_LOCKDURATION) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_LOCKDURATION, JONESRETICLE_DEFAULT_LOCKDURATION);
    }

    JonesReticle_reticle.unlockEndScale = stdConfig_GetFloat(JONESRETICLE_CFG_UNLOCKSCALE, JONESRETICLE_DEFAULT_UNLOCKSCALE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_UNLOCKSCALE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_UNLOCKSCALE, JONESRETICLE_DEFAULT_UNLOCKSCALE);
    }

    JonesReticle_reticle.unlockDuration = stdConfig_GetFloat(JONESRETICLE_CFG_UNLOCKDURATION, JONESRETICLE_DEFAULT_UNLOCKDURATION);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_UNLOCKDURATION) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_UNLOCKDURATION, JONESRETICLE_DEFAULT_UNLOCKDURATION);
    }

    float rotationSpeedDeg = stdConfig_GetFloat(JONESRETICLE_CFG_ROTATIONSPEED, JONESRETICLE_DEFAULT_ROTATIONSPEED);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_ROTATIONSPEED) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_ROTATIONSPEED, JONESRETICLE_DEFAULT_ROTATIONSPEED);
    }
    JonesReticle_reticle.rotationSpeed = STDMATH_RADIANSF(rotationSpeedDeg);

    JonesReticle_reticle.pulseMagnitude = stdConfig_GetFloat(JONESRETICLE_CFG_PULSEMAGNITUDE, JONESRETICLE_DEFAULT_PULSEMAGNITUDE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_PULSEMAGNITUDE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_PULSEMAGNITUDE, JONESRETICLE_DEFAULT_PULSEMAGNITUDE);
    }

    tStdColor color = stdConfig_GetColor(JONESRETICLE_CFG_COLOR, JonesReticle_ColorFromVector4(&JONESRETICLE_DEFAULT_COLOR));
    JonesReticle_reticle.color = JonesReticle_ColorToVector4(color);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_COLOR) )
    {
        stdConfig_SetColor(JONESRETICLE_CFG_COLOR, JonesReticle_ColorFromVector4(&JONESRETICLE_DEFAULT_COLOR));
    }

    JonesReticle_reticle.minDistance = stdConfig_GetFloat(JONESRETICLE_CFG_MINDISTANCE, JONESRETICLE_DEFAULT_MINDISTANCE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_MINDISTANCE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_MINDISTANCE, JONESRETICLE_DEFAULT_MINDISTANCE);
    }

    JonesReticle_reticle.maxDistance = stdConfig_GetFloat(JONESRETICLE_CFG_MAXDISTANCE, JONESRETICLE_DEFAULT_MAXDISTANCE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_MAXDISTANCE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_MAXDISTANCE, JONESRETICLE_DEFAULT_MAXDISTANCE);
    }

    JonesReticle_reticle.minDistanceScale = stdConfig_GetFloat(JONESRETICLE_CFG_MINDISTANCESCALE, JONESRETICLE_DEFAULT_MINDISTANCESCALE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_MINDISTANCESCALE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_MINDISTANCESCALE, JONESRETICLE_DEFAULT_MINDISTANCESCALE);
    }

    JonesReticle_reticle.maxDistanceScale = stdConfig_GetFloat(JONESRETICLE_CFG_MAXDISTANCESCALE, JONESRETICLE_DEFAULT_MAXDISTANCESCALE);
    if ( !stdConfig_Contains(JONESRETICLE_CFG_MAXDISTANCESCALE) )
    {
        stdConfig_SetFloat(JONESRETICLE_CFG_MAXDISTANCESCALE, JONESRETICLE_DEFAULT_MAXDISTANCESCALE);
    }

    // Mark module open
    JonesReticle_bOpen = true;
}
void JonesReticle_Close(void)
{
    if ( !JonesReticle_bOpen )
    {
        return;
    }

    STD_ZEROMEM(&JonesReticle_reticle, sizeof(JonesReticle_reticle));
    JonesReticle_bOpen = false;
}

void JonesReticle_Enable(bool bEnable)
{
    if ( JonesReticle_reticle.bEnabled != bEnable )
    {
        JonesReticle_reticle.bEnabled = bEnable;
        stdConfig_SetBool(JONESRETICLE_CFG_ENABLED, bEnable);
    }
}

bool JonesReticle_IsEnabled(void)
{
    return JonesReticle_reticle.bEnabled;
}


/**
* Calculate easing function for smooth animation(ease - out)
*/
static inline float J3DAPI JonesReticle_EaseOut(float t)
{
    // Ease out cubic: 1 - (1-t)^3
    float oneMinusT = 1.0f - t;
    return 1.0f - (oneMinusT * oneMinusT * oneMinusT);
}

/**
* Project 3D world position to screen space using camera
* Returns true if successful, false if behind camera or off-screen
*/
bool J3DAPI JonesReticle_ProjectToScreen(const rdVector3* pWorldPos, const rdCamera* pCamera, JonesReticleScreenPos* pScreenPosOut)
{
    if ( !pWorldPos || !pCamera || !pScreenPosOut )
    {
        return false;
    }

    // Transform world space to view space
    rdVector3 viewPos;
    rdMatrix_TransformPoint34(&viewPos, pWorldPos, &pCamera->viewMatrix);

    // Check if behind camera
    if ( viewPos.y <= 0.0f )
    {
        pScreenPosOut->bVisible = false;
        return false;
    }

    // Project to screen space using camera's projection function
    rdVector3 screenPos;
    pCamera->pfProject(&screenPos, &viewPos);

    // Check if within screen bounds
    // Note: screenPos should now be in screen coordinates
    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);
    if ( screenPos.x < 0.0f || screenPos.x > width ||
        screenPos.y < 0.0f || screenPos.y > height )
    {
        pScreenPosOut->bVisible = false;
        return false;
    }

    // Store screen position
    pScreenPosOut->sx  = screenPos.x;
    pScreenPosOut->sy  = screenPos.y;
    pScreenPosOut->bVisible = true;

    return true;
}

// Draw a single reticle arrow (triangle)
// direction: 0=top, 1=right, 2=bottom, 3=left
static void J3DAPI JonesReticle_DrawArrow(const JonesAimReticle* reticle, float screenX, float screenY, float radius, float arrowSize, const rdVector4* pColor, int direction)
{
    rdCacheProcEntry* pPoly = NULL;

    bool bAlpha = pColor->alpha != 1.0f;
    if ( bAlpha )
    {
        pPoly = rdCache_GetAlphaProcEntry();
        pPoly->flags = RD_FF_TEX_TRANSLUCENT | RD_FF_ZWRITE_DISABLED;
    }
    else
    {
        pPoly = rdCache_GetProcEntry();
        pPoly->flags = 0;
    }

    if ( !pPoly )
    {
        return;
    }

    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->numVertices  = 3;    // Triangle
    pPoly->pMaterial    = NULL; // No texture, solid color
    pPoly->matCelNum    = 0;
    pPoly->extraLight   = *pColor;

    // Apply pulse animation
    float animRadius = radius;
    if ( reticle->pulseTime > 0.0f && reticle->state == JONESRETICLE_STATE_LOCKED )
    {
        animRadius += sinf(reticle->pulseTime * 2 * STDMATH_PI_F) * (arrowSize * reticle->pulseMagnitude);
    }

    float halfSize = (arrowSize * 0.5f) * reticle->arrowWidthScale;

    // Calculate base angle for this arrow (0=top, 90=right, 180=bottom, 270=left)
    float angle = direction * STDMATH_RADIANSF(90.0); // 90 degrees
    angle += reticle->rotationAngle;

    // Calculate rotated position for arrow tip
    float cosAngle = cosf(angle);
    float sinAngle = sinf(angle);

    // Arrow tip (points toward center)
    float tipX = screenX + cosAngle * animRadius;
    float tipY = screenY + sinAngle * animRadius;

    // Calculate perpendicular vector for arrow base
    float perpX = -sinAngle;
    float perpY = cosAngle;

    // Arrow extends outward from tip
    float baseX = tipX + cosAngle * arrowSize;
    float baseY = tipY + sinAngle * arrowSize;

    // Triangle vertices: tip and two base corners
    pPoly->aVertices[0].sx = tipX;
    pPoly->aVertices[0].sy = tipY;

    pPoly->aVertices[1].sx = baseX - perpX * halfSize;
    pPoly->aVertices[1].sy = baseY - perpY * halfSize;

    pPoly->aVertices[2].sx = baseX + perpX * halfSize;
    pPoly->aVertices[2].sy = baseY + perpY * halfSize;

    // Set common vertex properties
    for ( size_t i = 0; i < pPoly->numVertices; i++ )
    {
        pPoly->aVertices[i].sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
        pPoly->aVertices[i].rhw = RD_FIXEDPOINT_RHW_SCALE_X5;
        pPoly->aVertices[i].tu  = 0.0f;
        pPoly->aVertices[i].tv  = 0.0f;

        // Set vertex intensities
        pPoly->aVertIntensities[i] = *pColor;
    }

    // Submit to renderer
    if ( bAlpha )
    {
        rdCache_AddAlphaProcFace(3);
    }
    else
    {
        rdCache_AddProcFace(3);
    }
}

// Calculate scale factor based on distance to target
static float J3DAPI JonesReticle_CalcDistanceScale(const JonesAimReticle* reticle, const rdVector3* pTargetPos)
{
    if ( !rdCamera_g_pCurCamera )
    {
        return 1.0f;
    }

    // Calculate distance from camera to target
    rdVector3 camPos = rdCamera_g_camMatrix.dvec;
    float distance = rdVector_Dist3(pTargetPos, &camPos);

    // Clamp distance to min/max range
    if ( distance <= reticle->minDistance )
    {
        return reticle->minDistanceScale;
    }

    if ( distance >= reticle->maxDistance )
    {
        return reticle->maxDistanceScale;
    }

    // inverse-distance interpolation
    float invMin  = 1.0f / reticle->maxDistance;
    float invMax  = 1.0f / reticle->minDistance;
    float invDist = 1.0f / distance;

    // remap invDist from [invMax .. invMin] to [0..1]
    float t = (invDist - invMax) / (invMin - invMax);

    // lerp scale between minScale..maxScale (or vice versa)
    return reticle->minDistanceScale + (reticle->maxDistanceScale - reticle->minDistanceScale) * t;
}

// Main function to draw the complete lock-on reticle with animation
void J3DAPI JonesReticle_Draw(const JonesAimReticle* reticle, const JonesReticleScreenPos* pScreenPos)
{
    if ( !reticle || !pScreenPos || !pScreenPos->bVisible )
    {
        return;
    }

    if ( reticle->state == JONESRETICLE_STATE_INACTIVE )
    {
        return;
    }

    // Calculate scale based on animation state
    float animScale           = 1.0f;
    float alphaMultiplier = 1.0f;

    switch ( reticle->state )
    {
        case JONESRETICLE_STATE_LOCKING:
        {
            // Scale from large to normal size
            float t          = JonesReticle_EaseOut(reticle->animTime);
            float scaleRange = reticle->lockStartScale - 1.0f;
            animScale        = reticle->lockStartScale - (scaleRange * t); // Shrink
            alphaMultiplier  = t; // Fade in
        }
        break;

        case JONESRETICLE_STATE_LOCKED:
            animScale           = 1.0f;
            alphaMultiplier = 1.0f;
            break;

        case JONESRETICLE_STATE_UNLOCKING:
        {
            // Scale from normal to large and fade out
            float t          = JonesReticle_EaseOut(reticle->animTime);
            float scaleRange = reticle->unlockEndScale - 1.0f;
            animScale        = 1.0f + (scaleRange * t); // Grow
            alphaMultiplier  = 1.0f - t; // Fade out
        }
        break;

        default:
            break;
    }

    // Calculate screen res scale factor
    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);
    float screenScaleFactor = (float)height / RD_REF_HEIGHT;

    // Get distance-based scale
    float distanceScale = JonesReticle_CalcDistanceScale(reticle, &reticle->targetPos);

    // Combine all scale factors: base size * screen resolution scale * animation scale * distance scale
    float finalScale = animScale * screenScaleFactor * distanceScale;


    // Calculate current size
    float currentRadius    = reticle->baseRadius * finalScale;
    float currentArrowSize = reticle->baseArrowSize * finalScale;

    // Get color
    rdVector4 color = reticle->color;
    color.alpha *= alphaMultiplier;

    // Draw all 4 arrows in cross formation
    for ( int i = 0; i < 4; i++ )
    {
        JonesReticle_DrawArrow(reticle,
            pScreenPos->sx,
            pScreenPos->sy,
            currentRadius,
            currentArrowSize,
            &color,
            i
        );
    }

    rdCache_Flush();
    rdCache_FlushAlpha();
}


// Start lock-on animation
void J3DAPI JonesReticle_StartLockOn(JonesAimReticle* reticle)
{
    if ( !reticle )
    {
        return;
    }

    reticle->state     = JONESRETICLE_STATE_LOCKING;
    reticle->animTime  = 0.0f;
    reticle->pulseTime = 0.0f;
}

// Start unlock animation
void J3DAPI JonesReticle_StartUnlock(JonesAimReticle* reticle)
{
    if ( !reticle )
    {
        return;
    }

    reticle->state    = JONESRETICLE_STATE_UNLOCKING;
    reticle->animTime = 0.0f;
}

// Initialize reticle with default values
void J3DAPI JonesReticle_SetTarget(JonesAimReticle* reticle, const SithThing* pTargetThing)
{
    if ( !reticle )
    {
        return;
    }

    reticle->pTarget       = pTargetThing;

    reticle->bPlaying      = true;
    reticle->state         = JONESRETICLE_STATE_INACTIVE;
    reticle->animTime      = 0.0f;
    reticle->pulseTime     = 0.0f;

    reticle->rotationAngle = 0.0f;
}

// Update reticle animation state
void J3DAPI JonesReticle_Update(float deltaTime)
{
    if ( !JonesReticle_reticle.bEnabled )
    {
        return;
    }

    // Update target
    SithThing* pCurPlayerTarget = sithPlayerControls_GetTargetThing();
    if ( pCurPlayerTarget != JonesReticle_reticle.pTarget )
    {
        if ( pCurPlayerTarget )
        {

            JonesReticle_SetTarget(&JonesReticle_reticle, pCurPlayerTarget);
            JonesReticle_StartLockOn(&JonesReticle_reticle); // Start animation
        }
        else
        {
            JonesReticle_StartUnlock(&JonesReticle_reticle);
            JonesReticle_reticle.pTarget = NULL;
        }
    }

    // Skip further updating reticle if inactive
    if ( JonesReticle_reticle.state == JONESRETICLE_STATE_INACTIVE )
    {
        return;
    }

    // Update target position
    if ( JonesReticle_reticle.pTarget )
    {
        JonesReticle_reticle.targetPos = JonesReticle_reticle.pTarget->pos;
    }

    // Skip updating animation
    if ( !JonesReticle_reticle.bPlaying )
    {
        return;
    }

    // Update rotation
    JonesReticle_reticle.rotationAngle += JonesReticle_reticle.rotationSpeed * deltaTime;
    if ( JonesReticle_reticle.rotationAngle > 2 * STDMATH_PI_F )
    {
        JonesReticle_reticle.rotationAngle -= 2 * STDMATH_PI_F;
    }

    switch ( JonesReticle_reticle.state )
    {
        case JONESRETICLE_STATE_LOCKING:
            JonesReticle_reticle.animTime += deltaTime / JonesReticle_reticle.lockDuration;
            if ( JonesReticle_reticle.animTime >= 1.0f )
            {
                JonesReticle_reticle.animTime = 1.0f;
                JonesReticle_reticle.state    = JONESRETICLE_STATE_LOCKED;
            }
            break;

        case JONESRETICLE_STATE_UNLOCKING:
            JonesReticle_reticle.animTime += deltaTime / JonesReticle_reticle.unlockDuration;
            if ( JonesReticle_reticle.animTime >= 1.0f )
            {
                JonesReticle_reticle.animTime = 0.0f;
                JonesReticle_reticle.state    = JONESRETICLE_STATE_INACTIVE;
            }
            break;

        case JONESRETICLE_STATE_LOCKED:
            // Update pulse animation
            JonesReticle_reticle.pulseTime += deltaTime;
            break;

        default:
            break;
    }
}

void JonesReticle_Render(void)
{
    if ( !JonesReticle_reticle.bEnabled || JonesReticle_reticle.state == JONESRETICLE_STATE_INACTIVE )
    {
        return;
    }

    // Use global camera
    if ( !rdCamera_g_pCurCamera )
    {
        return;
    }

    // Project target to screen
    JonesReticleScreenPos screenPos;
    if ( JonesReticle_ProjectToScreen(&JonesReticle_reticle.targetPos, rdCamera_g_pCurCamera, &screenPos) )
    {
        JonesReticle_Draw(&JonesReticle_reticle, &screenPos);
    }
}

bool JonesReticle_IsPaused(void)
{
    return !JonesReticle_reticle.bPlaying;
}

void JonesReticle_Pause(void)
{
    JonesReticle_reticle.bPlaying = false;
}

void JonesReticle_Resume(void)
{
    JonesReticle_reticle.bPlaying = true;
}