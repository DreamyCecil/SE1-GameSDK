/* Copyright (c) 2002-2012 Croteam Ltd.
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "stdafx.h"
#include "Camera.h"

class CCameraPos {
  public:
  FTICK cp_ftTick;
  FLOAT cp_fSpeed;
  FLOAT3D cp_vPos;
  ANGLE3D cp_aRot;
  ANGLE cp_aFOV;
};

BOOL _bCameraOn = FALSE;
CTFileStream _strScript;
BOOL _bInitialized;
FTICK _ftStartTime;
CCameraPos _cp0;
CCameraPos _cp1;
CCameraPos _cp;

// camera control
extern INDEX cam_bRecord = FALSE;
static INDEX cam_bMoveForward = FALSE;
static INDEX cam_bMoveBackward = FALSE;
static INDEX cam_bMoveLeft = FALSE;
static INDEX cam_bMoveRight = FALSE;
static INDEX cam_bMoveUp = FALSE;
static INDEX cam_bMoveDown = FALSE;
static INDEX cam_bTurnBankingLeft = FALSE;
static INDEX cam_bTurnBankingRight = FALSE;
static INDEX cam_bZoomIn = FALSE;
static INDEX cam_bZoomOut = FALSE;
static INDEX cam_bZoomDefault = FALSE;
static INDEX cam_bResetToPlayer = FALSE;
static INDEX cam_bSnapshot = FALSE;
static INDEX cam_fSpeed = 1.0f;

// camera functions
void CAM_Init(void) {
  _pShell->DeclareSymbol("user INDEX cam_bRecord;", &cam_bRecord);
  _pShell->DeclareSymbol("user INDEX cam_bMoveForward;", &cam_bMoveForward);
  _pShell->DeclareSymbol("user INDEX cam_bMoveBackward;", &cam_bMoveBackward);
  _pShell->DeclareSymbol("user INDEX cam_bMoveLeft;", &cam_bMoveLeft);
  _pShell->DeclareSymbol("user INDEX cam_bMoveRight;", &cam_bMoveRight);
  _pShell->DeclareSymbol("user INDEX cam_bMoveUp;", &cam_bMoveUp);
  _pShell->DeclareSymbol("user INDEX cam_bMoveDown;", &cam_bMoveDown);
  _pShell->DeclareSymbol("user INDEX cam_bTurnBankingLeft;", &cam_bTurnBankingLeft);
  _pShell->DeclareSymbol("user INDEX cam_bTurnBankingRight;", &cam_bTurnBankingRight);
  _pShell->DeclareSymbol("user INDEX cam_bZoomIn;", &cam_bZoomIn);
  _pShell->DeclareSymbol("user INDEX cam_bZoomOut;", &cam_bZoomOut);
  _pShell->DeclareSymbol("user INDEX cam_bZoomDefault;", &cam_bZoomDefault);
  _pShell->DeclareSymbol("user INDEX cam_bSnapshot;", &cam_bSnapshot);
  _pShell->DeclareSymbol("user INDEX cam_bResetToPlayer;", &cam_bResetToPlayer);
  _pShell->DeclareSymbol("user INDEX cam_fSpeed;", &cam_fSpeed);
}

BOOL CAM_IsOn(void) {
  return _bCameraOn;
}

void ReadPos(CCameraPos &cp) {
  try {
    // [Cecil] New timer: Separate ticks from the fraction
    TICK llTick;
    FLOAT fTick;

    CTString strLine;
    _strScript.GetLine_t(strLine);
    strLine.ScanF("%g, %g: %g: %g %g %g:%g %g %g:%g", &llTick, &fTick, &cp.cp_fSpeed, &cp.cp_vPos(1), &cp.cp_vPos(2),
                  &cp.cp_vPos(3), &cp.cp_aRot(1), &cp.cp_aRot(2), &cp.cp_aRot(3), &cp.cp_aFOV);

    cp.cp_ftTick = FTICK(llTick, fTick);

  } catch (char *strError) {
    CPrintF("Camera: %s\n", strError);
  }
}

void WritePos(CCameraPos &cp) {
  try {
    // [Cecil] New timer: Separate ticks from the fraction
    TICK llTick = (_pTimer->LerpedGameTick() - _ftStartTime).llTick;
    FLOAT fTick = (_pTimer->LerpedGameTick() - _ftStartTime).fFrac;

    CTString strLine;
    strLine.PrintF("%g, %g: %g: %g %g %g:%g %g %g:%g", llTick, fTick, 1.0f, cp.cp_vPos(1), cp.cp_vPos(2), cp.cp_vPos(3),
                   cp.cp_aRot(1), cp.cp_aRot(2), cp.cp_aRot(3), cp.cp_aFOV);
    _strScript.PutLine_t(strLine);

  } catch (char *strError) {
    CPrintF("Camera: %s\n", strError);
  }
}

void SetSpeed(FLOAT fSpeed) {
  CTString str;
  str.PrintF("dem_fRealTimeFactor = %g;", fSpeed);
  _pShell->Execute(str);
}

void CAM_Start(const CTFileName &fnmDemo) {
  _bCameraOn = FALSE;
  CTFileName fnmScript = fnmDemo.NoExt() + ".ini";

  if (cam_bRecord) {
    try {
      _strScript.Create_t(fnmScript);
    } catch (char *strError) {
      CPrintF("Camera: %s\n", strError);
      return;
    }

    _cp.cp_vPos = FLOAT3D(0.0f, 0.0f, 0.0f);
    _cp.cp_aRot = ANGLE3D(0.0f, 0.0f, 0.0f);
    _cp.cp_aFOV = 90.0f;
    _cp.cp_fSpeed = 1;
    _cp.cp_ftTick = 0;

  } else {
    try {
      _strScript.Open_t(fnmScript);
    } catch (char *strError) {
      (void)strError;
      return;
    }
  }

  _bCameraOn = TRUE;
  _bInitialized = FALSE;
}

void CAM_Stop(void) {
  if (_bCameraOn) {
    _strScript.Close();
  }
  _bCameraOn = FALSE;
}

void CAM_Render(CEntity *pen, CDrawPort *pdp) {
  if (cam_bRecord) {
    if (!_bInitialized) {
      _bInitialized = TRUE;
      SetSpeed(1.0f);
      _ftStartTime = _pTimer->GetGameTick();
    }

    FLOATmatrix3D m;
    MakeRotationMatrixFast(m, _cp.cp_aRot);
    FLOAT3D vX, vY, vZ;
    vX(1) = m(1, 1);
    vX(2) = m(2, 1);
    vX(3) = m(3, 1);
    vY(1) = m(1, 2);
    vY(2) = m(2, 2);
    vY(3) = m(3, 2);
    vZ(1) = m(1, 3);
    vZ(2) = m(2, 3);
    vZ(3) = m(3, 3);

    _cp.cp_aRot(1) -= _pInput->GetAxisValue(MOUSE_X_AXIS) * 0.5f;
    _cp.cp_aRot(2) -= _pInput->GetAxisValue(MOUSE_Y_AXIS) * 0.5f;

    if (cam_bMoveForward) {
      _cp.cp_vPos -= vZ * cam_fSpeed;
    }

    if (cam_bMoveBackward) {
      _cp.cp_vPos += vZ * cam_fSpeed;
    }

    if (cam_bMoveLeft) {
      _cp.cp_vPos -= vX * cam_fSpeed;
    }

    if (cam_bMoveRight) {
      _cp.cp_vPos += vX * cam_fSpeed;
    }

    if (cam_bMoveUp) {
      _cp.cp_vPos += vY * cam_fSpeed;
    }

    if (cam_bMoveDown) {
      _cp.cp_vPos -= vY * cam_fSpeed;
    }

    if (cam_bTurnBankingLeft) {
      _cp.cp_aRot(3) += 10.0f;
    }

    if (cam_bTurnBankingRight) {
      _cp.cp_aRot(3) -= 10.0f;
    }

    if (cam_bZoomIn) {
      _cp.cp_aFOV -= 1.0f;
    }

    if (cam_bZoomOut) {
      _cp.cp_aFOV += 1.0f;
    }

    if (cam_bZoomDefault) {
      _cp.cp_aFOV = 90.0f;
    }

    Clamp(_cp.cp_aFOV, 10.0f, 150.0f);

    if (cam_bResetToPlayer) {
      _cp.cp_vPos = pen->GetPlacement().pl_PositionVector;
      _cp.cp_aRot = pen->GetPlacement().pl_OrientationAngle;
    }

    if (cam_bSnapshot) {
      cam_bSnapshot = FALSE;
      WritePos(_cp);
    }

  } else {
    if (!_bInitialized) {
      _bInitialized = TRUE;
      ReadPos(_cp0);
      ReadPos(_cp1);
      SetSpeed(_cp0.cp_fSpeed);
      _ftStartTime = _pTimer->GetGameTick();
    }

    FTICK ftNow = _pTimer->LerpedGameTick() - _ftStartTime;
    if (ftNow > _cp1.cp_ftTick) {
      _cp0 = _cp1;
      ReadPos(_cp1);
      SetSpeed(_cp0.cp_fSpeed);
    }

    FLOAT fRatio = CTimer::InSeconds(ftNow - _cp0.cp_ftTick) / CTimer::InSeconds(_cp1.cp_ftTick - _cp0.cp_ftTick);

    _cp.cp_vPos = Lerp(_cp0.cp_vPos, _cp1.cp_vPos, fRatio);
    _cp.cp_aRot = Lerp(_cp0.cp_aRot, _cp1.cp_aRot, fRatio);
    _cp.cp_aFOV = Lerp(_cp0.cp_aFOV, _cp1.cp_aFOV, fRatio);
  }

  CPlacement3D plCamera;
  plCamera.pl_PositionVector = _cp.cp_vPos;
  plCamera.pl_OrientationAngle = _cp.cp_aRot;

  // init projection parameters
  CPerspectiveProjection3D prPerspectiveProjection;
  prPerspectiveProjection.FOVL() = _cp.cp_aFOV;
  prPerspectiveProjection.ScreenBBoxL()
    = FLOATaabbox2D(FLOAT2D(0.0f, 0.0f), FLOAT2D((float)pdp->GetWidth(), (float)pdp->GetHeight()));
  prPerspectiveProjection.AspectRatioL() = 1.0f;
  prPerspectiveProjection.FrontClipDistanceL() = 0.3f;

  CAnyProjection3D prProjection;
  prProjection = prPerspectiveProjection;

  // set up viewer position
  prProjection->ViewerPlacementL() = plCamera;
  // render the view
  RenderView(*pen->en_pwoWorld, *(CEntity *)NULL, prProjection, *pdp);
}
