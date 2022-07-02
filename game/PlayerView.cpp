// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Player.h"
#include "PlayerView.h"
#include "rules/GameRules.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleView.h"
#include "guis/UserInterfaceLocal.h"
#include "Weapon.h"
#include "demos/DemoManager.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "guis/GuiSurface.h"
#include "misc/WorldToScreen.h"
#include "roles/WayPointManager.h"
#include "Atmosphere.h"
#include "client/ClientEffect.h"

const int IMPULSE_DELAY = 150;

/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView( void ) {
	memset( &currentView, 0, sizeof( currentView ) );
	kickFinishTime = 0;
	kickAngles.Zero();
	swayAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();
	shakeAng.Zero();
	shakeAxes.Identity();
	lastSpectateUpdateTime = 0;

	ClearEffects();
}

/*
==============
idPlayerView::~idPlayerView
==============
*/
idPlayerView::~idPlayerView( void ) {
}

/*
==============
idPlayerView::ClearEffects
==============
*/
void idPlayerView::ClearEffects() {
	lastDamageTime = 0;

	kickFinishTime = 0;

	fadeTime = 0;

	underWaterEffect.StopDetach();
}

/*
==============
idPlayerView::DamageImpulse

LocalKickDir is the direction of force in the player's coordinate system,
which will determine the head kick direction
==============
*/
void idPlayerView::DamageImpulse( idVec3 localKickDir, const sdDeclDamage* damageDecl ) {
	//
	// double vision effect
	//
	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	//
	// head angle kick
	//
	float kickTime = damageDecl->GetKickTime();
	if ( kickTime ) {
		kickFinishTime = gameLocal.time + static_cast< int >( g_kickTime.GetFloat() * kickTime );

		// forward / back kick will pitch view
		kickAngles[0] = localKickDir[0];

		// side kick will yaw view
		kickAngles[1] = localKickDir[1]*0.5f;

		// up / down kick will pitch view
		kickAngles[0] += localKickDir[2];

		// roll will come from  side
		kickAngles[2] = localKickDir[1];

		float kickAmplitude = damageDecl->GetKickAmplitude();
		if ( kickAmplitude ) {
			kickAngles *= kickAmplitude;
		}
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.time );

}

/*
==================
idPlayerView::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayerView::WeaponFireFeedback( const weaponFeedback_t& feedback ) {
	// don't shorten a damage kick in progress
	if ( feedback.recoilTime != 0 && kickFinishTime < gameLocal.time ) {
		kickAngles = feedback.recoilAngles;
		int	finish = gameLocal.time + static_cast< int >( g_kickTime.GetFloat() * feedback.recoilTime );
		kickFinishTime = finish;
	}	

}

/*
===================
idPlayerView::SetActiveProxyView
===================
*/
void idPlayerView::SetActiveProxyView( idEntity* other, idPlayer* player ) {
	if ( activeViewProxy.IsValid() ) {
		activeViewProxy->GetUsableInterface()->StopActiveViewProxy();
		activeViewProxy = NULL;

		gameLocal.localPlayerProperties.OnActiveViewProxyChanged( NULL );
	}

	if ( other ) {
		other->GetUsableInterface()->BecomeActiveViewProxy( player );
		activeViewProxy = other;

		gameLocal.localPlayerProperties.OnActiveViewProxyChanged( other );
	}
}

/*
===================
idPlayerView::UpdateProxyView
===================
*/
void idPlayerView::UpdateProxyView( idPlayer* player, bool force ) {
	idEntity* proxy = player->GetProxyEntity();

	if ( activeViewProxy != proxy || force ) {
		SetActiveProxyView( proxy, player );
	}

	if ( activeViewProxy.IsValid() ) {
		activeViewProxy->GetUsableInterface()->UpdateProxyView( player );
	}
}

/*
===================
idPlayerView::CalculateShake
===================
*/
void idPlayerView::CalculateShake( idPlayer* other ) {
	idVec3	origin, matrix;

	float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.time, other->firstPersonViewOrigin );

	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//

	if( shakeVolume ) {
		shakeAng[0] = gameLocal.random.CRandomFloat() * shakeVolume;
		shakeAng[1] = gameLocal.random.CRandomFloat() * shakeVolume;
		shakeAng[2] = gameLocal.random.CRandomFloat() * shakeVolume;
		shakeAxes = shakeAng.ToMat3();
	} else {
		shakeAng.Zero();
		shakeAxes.Identity();
	}
}

/*
===================
idPlayerView::ShakeAxis
===================
*/
const idMat3& idPlayerView::ShakeAxis( void ) const {
	return shakeAxes;
}

/*
===================
idPlayerView::AngleOffset
===================
*/
idAngles idPlayerView::AngleOffset( const idPlayer* player ) const {
	idAngles angles;

	angles.Zero();

	if ( gameLocal.time < kickFinishTime ) {
		int offset = kickFinishTime - gameLocal.time;

		angles = kickAngles * static_cast< float >( Square( offset ) ) * g_kickAmplitude.GetFloat();

		for ( int i = 0 ; i < 3 ; i++ ) {
			if ( angles[i] > 70.0f ) {
				angles[i] = 70.0f;
			} else if ( angles[i] < -70.0f ) {
				angles[i] = -70.0f;
			}
		}
	}

	if ( player ) {
		const idWeapon* weapon = player->GetWeapon();
		if ( weapon ) {
			weapon->SwayAngles( angles );
		}
	}

	return angles;
}

/*
==================
idPlayerView::SkipWorldRender
==================
*/
bool idPlayerView::SkipWorldRender( void ) {
	bool skipWorldRender =  false;
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		sdHudModule* module = NULL;		
		for ( module = gameLocal.localPlayerProperties.GetDrawHudModule(); module; module = module->GetDrawNode().Next() ) {
			const sdUserInterfaceLocal* ui = module->GetGui();
			if( ui == NULL ) {
				continue;
			}
			if( ui->TestGUIFlag( sdUserInterfaceLocal::GUI_INHIBIT_GAME_WORLD ) ) {
				skipWorldRender = true;
				break;
			}			
		}
		if ( sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( gameLocal.GetScoreBoardGui() ) ) {
			skipWorldRender |= scoreboardUI->TestGUIFlag( sdUserInterfaceLocal::GUI_INHIBIT_GAME_WORLD );
		}
	}
	return skipWorldRender;
}

/*
==================
idPlayerView::SingleView
==================
*/
void idPlayerView::SingleView( idPlayer* viewPlayer, const renderView_t* view ) {

	// normal rendering
	if ( view == NULL ) {
		return;
	}

	bool runEffect = false;
	idVec3 const &viewOrg = viewPlayer->GetRenderView()->vieworg;
	if ( !g_skipLocalizedPrecipitation.GetBool() ) {

		idBounds checkBounds( viewOrg );
		checkBounds.ExpandSelf( 8.0f );
		const idClipModel* waterModel;
		int found = gameLocal.clip.FindWater( CLIP_DEBUG_PARMS checkBounds, &waterModel, 1 );
		int cont = 0;
		if ( found ) {
			cont = gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS viewOrg, NULL, mat3_identity, CONTENTS_WATER, waterModel, waterModel->GetOrigin(), waterModel->GetAxis() );
		}

		runEffect = (cont & CONTENTS_WATER) ? true : false;
	}
	// Update the background effect
	if ( runEffect ) {
		underWaterEffect.GetRenderEffect().origin = viewOrg;
		if ( !underWaterEffectRunning ) {
			SetupEffect();
			underWaterEffect.Start( gameLocal.time );
			underWaterEffectRunning = true;
		} else {
			underWaterEffect.Update();
		}
	} else {
		underWaterEffect.StopDetach();
		underWaterEffectRunning = false;
	}


	if ( viewPlayer != NULL ) {
		sdClientAnimated* sight = viewPlayer->GetSight();
		if ( sight != NULL ) {
			float fovx, fovy;
			gameLocal.CalcFov( viewPlayer->GetSightFOV(), fovx, fovy );
			sight->GetRenderEntity()->weaponDepthHackFOV_x = fovx;
			sight->GetRenderEntity()->weaponDepthHackFOV_y = fovy;

			sight->SetOrigin( view->vieworg );
			sight->SetAxis( view->viewaxis );
			sight->Present();
		}
	}

	if ( !SkipWorldRender() ) {
		idFrustum viewFrustum;

		BuildViewFrustum( view, viewFrustum );

		// emit render commands for in-world guis
		DrawWorldGuis( view, viewFrustum );

		gameRenderWorld->UpdatePortalOccTestView( view->viewID );

		UpdateOcclusionQueries( view );

		// draw the game
		gameRenderWorld->RenderScene( view );
	}

	sdDemoManager::GetInstance().SetRenderedView( *view );
}

/*
==================
idPlayerView::SingleView2D
==================
*/
void idPlayerView::SingleView2D( idPlayer* viewPlayer ) {
	sdPostProcess* postProcess = gameLocal.localPlayerProperties.GetPostProcess();
	postProcess->DrawPost();		

	if ( sdDemoManager::GetInstance().g_showDemoHud.GetBool() && sdDemoManager::GetInstance().InPlayBack() ) {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( sdDemoManager::GetInstance().GetHudHandle() );

		if ( ui != NULL ) {
			ui->Draw();
		}
	}	

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		bool drawHud = !localPlayer->InhibitHud();

		idEntity* proxy = viewPlayer->GetProxyEntity();
		if ( proxy != NULL ) {
			drawHud &= !proxy->GetUsableInterface()->GetHideHud( viewPlayer );
		}
		
		sdHudModule* module = NULL;		
		for ( module = gameLocal.localPlayerProperties.GetDrawHudModule(); module; module = module->GetDrawNode().Next() ) {
			if( module->ManualDraw() ) {
				continue;
			}
			if( drawHud || !module->AllowInhibit() ) {
				module->Draw();
			}			
		}
			// jrad - the scoreboard should top everything
		if( drawHud ) {
			DrawScoreBoard( localPlayer );
		}		
	}

	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] ) {
		const idMaterial* mtr = declHolder.declMaterialType.LocalFind( g_testPostProcess.GetString(), false );
		if ( mtr == NULL ) {
			gameLocal.Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		} else {
			deviceContext->SetColor( 1.0f, 1.0f, 1.0f, 1.0f );
			deviceContext->DrawRect( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}

	if ( !g_skipViewEffects.GetBool() ) {
		ScreenFade();
	}
}

/*
=================
idPlayerView::Flash

flashes the player view with the given color
=================
*/
void idPlayerView::Flash( const idVec4& color, int time ) {
	Fade( idVec4( 0.0f, 0.0f, 0.0f, 0.0f ), time );
	fadeFromColor = color;
}

/*
=================
idPlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void idPlayerView::Fade( const idVec4& color, int time ) {

	if ( !fadeTime ) {
		fadeFromColor.Set( 0.0f, 0.0f, 0.0f, 1.0f - color[ 3 ] );
	} else {
		fadeFromColor = fadeColor;
	}
	fadeToColor = color;

	if ( time <= 0 ) {
		fadeRate = 0;
		time = 0;
		fadeColor = fadeToColor;
	} else {
		fadeRate = 1.0f / ( float )time;
	}

	if ( gameLocal.realClientTime == 0 && time == 0 ) {
		fadeTime = 1;
	} else {
		fadeTime = gameLocal.realClientTime + time;
	}
}

/*
=================
idPlayerView::ScreenFade
=================
*/
void idPlayerView::ScreenFade() {
	int		msec;
	float	t;

	if ( !fadeTime ) {
		return;
	}

	msec = fadeTime - gameLocal.realClientTime;

	if ( msec <= 0 ) {
		fadeColor = fadeToColor;
		if ( fadeColor[ 3 ] == 0.0f ) {
			fadeTime = 0;
		}
	} else {
		t = ( float )msec * fadeRate;
		fadeColor = fadeFromColor * t + fadeToColor * ( 1.0f - t );
	}

	if ( fadeColor[ 3 ] != 0.0f ) {
		deviceContext->SetColor( fadeColor[ 0 ], fadeColor[ 1 ], fadeColor[ 2 ], fadeColor[ 3 ] );
		float aspectRatio = deviceContext->GetAspectRatioCorrection();
		deviceContext->DrawRect( 0, 0, SCREEN_WIDTH * 1.0f / aspectRatio, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, declHolder.declMaterialType.LocalFind( "_white" ) );
	}
}

/*
================
idPlayerView::DrawScoreBoard
================
*/
void idPlayerView::DrawScoreBoard( idPlayer* player ) {
	sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( gameLocal.GetScoreBoardGui() );
	if ( !scoreboardUI ) {
		return;
	}

	bool skipWorldRender = SkipWorldRender();
	bool showScores = sdDemoManager::GetInstance().InPlayBack() ? sdDemoManager::GetInstance().GetDemoCommand().clientButtons.showScores : player->usercmd.clientButtons.showScores;
	
	if ( ( showScores && !skipWorldRender ) || gameLocal.localPlayerProperties.ShouldShowEndGame() ) {
		if( !scoreboardUI->IsActive() ) {
			scoreboardUI->Activate();
			if( gameLocal.localPlayerProperties.ShouldShowEndGame() ) {
				sdChatMenu* menu = gameLocal.localPlayerProperties.GetChatMenu();
				if( menu->Enabled() ) {
					menu->Enable( false );
				}
				sdLimboMenu* limboMenu = gameLocal.localPlayerProperties.GetLimboMenu();
				if ( limboMenu->Enabled() ) {
					limboMenu->Enable( false );
				}
			}
		}		
		scoreboardUI->Draw();
	} else if( scoreboardUI->IsActive() ){
		scoreboardUI->Deactivate();
	}
}

/*
===================
idPlayerView::RenderPlayerView
===================
*/
bool idPlayerView::RenderPlayerView( idPlayer* viewPlayer ) {
	// hack the shake in at the very last moment, so it can't cause any consistency problems
	memset( &currentView, 0, sizeof( currentView ) );

	if ( viewPlayer == NULL ) {
		return false;
	}

	// allow demo manager to override the view
	if ( sdDemoManager::GetInstance().CalculateRenderView( &currentView ) ) {
		// place the sound origin for the player
		gameSoundWorld->PlaceListener( currentView.vieworg, currentView.viewaxis, -1, gameLocal.time );
		
		// field of view
		gameLocal.CalcFov( currentView.fov_x, currentView.fov_x, currentView.fov_y );
	} else {
		const renderView_t& view = viewPlayer->renderView;

		// hack the shake in at the very last moment, so it can't cause any consistency problems
		currentView = view;

		// readjust the view one frame earlier for regular game draws (again: for base draws, not for extra draws)
		// extra draws do this in their own loop
		if ( gameLocal.com_unlockFPS->GetBool() && !gameLocal.unlock.unlockedDraw && gameLocal.unlock.canUnlockFrames ) {
			if ( g_unlock_updateViewpos.GetBool() && g_unlock_viewStyle.GetInteger() == 1 ) {
				currentView.vieworg = gameLocal.unlock.originlog[ ( gameLocal.framenum + 1 ) & 1 ];
				if ( viewPlayer->weapon != NULL ) {
					viewPlayer->weapon->GetRenderEntity()->origin -= gameLocal.unlock.originlog[ gameLocal.framenum & 1 ] - gameLocal.unlock.originlog[ ( gameLocal.framenum + 1 ) & 1 ];
					viewPlayer->weapon->BecomeActive( TH_UPDATEVISUALS );
					viewPlayer->weapon->Present();

					// if the weapon has a GUI bound, offset it as well
					for ( rvClientEntity* cent = viewPlayer->weapon->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {

						sdGuiSurface* guiSurface;
						rvClientEffect* effect;
						if ( ( guiSurface = cent->Cast< sdGuiSurface >() ) != NULL ) {
							idVec3 o;
							guiSurface->GetRenderable().GetOrigin( o );
							o -= gameLocal.unlock.originlog[ gameLocal.framenum & 1 ] - gameLocal.unlock.originlog[ ( gameLocal.framenum + 1 ) & 1 ];
							guiSurface->GetRenderable().SetOrigin( o );
						} else if ( ( effect = cent->Cast< rvClientEffect >() ) != NULL ) {
							effect->ClientUpdateView();
						}
					}
				}
			}
		}

		// shakey shake
		currentView.viewaxis = currentView.viewaxis * ShakeAxis();

		viewPlayer->UpdateHudStats();

		// place the sound origin for the player
		gameSoundWorld->PlaceListener( view.vieworg, view.viewaxis, viewPlayer->entityNumber + 1, gameLocal.time );
	}

	// Special view mode is enabled
	if ( g_testViewSkin.GetString()[0] ) {
		const idDeclSkin *skin = declHolder.declSkinType.LocalFind( g_testViewSkin.GetString(), false );
		currentView.globalSkin = skin;
		if ( !skin ) {
			gameLocal.Printf( "Skin not found.\n" );
			g_testViewSkin.SetString( "" );
		}
	} else {
		if ( viewPlayer ) {
			currentView.globalSkin = viewPlayer->GetViewSkin();
		} else {
			currentView.globalSkin = NULL;
		}
	}

	currentView.forceClear = g_forceClear.GetBool();

	if ( sdAtmosphere::currentAtmosphere == NULL ) {
		currentView.clearColor.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	} else {
		sdAtmosphere	*atmos = sdAtmosphere::currentAtmosphere;
		idVec3 fogcol = atmos->GetFogColor();
		currentView.clearColor.Set( fogcol[0], fogcol[1], fogcol[2], 0.f );
	}

	SingleView( viewPlayer, &currentView );

	return true;
}

/*
===================
idPlayerView::RenderPlayerView2D
===================
*/
bool idPlayerView::RenderPlayerView2D( idPlayer* viewPlayer ) {
	SingleView2D( viewPlayer );

	return true;
}

/*
===================
idPlayerView::SetupCockpit
===================
*/
void idPlayerView::SetupCockpit( const idDict& info, idEntity* vehicle ) {
	ClearCockpit();

	const char* cockpitDefName = info.GetString( "def_cockpit" );
	const idDeclEntityDef* cockpitDef = gameLocal.declEntityDefType[ cockpitDefName ];
	if ( !cockpitDef ) {
		return;
	}

	const char* scriptObjectName = info.GetString( "scriptobject", "default" );

	cockpit = new sdClientAnimated();
	cockpit->Create( &cockpitDef->dict, gameLocal.program->FindTypeInfo( scriptObjectName ) );

	idScriptObject* scriptObject = cockpit->GetScriptObject();
	if ( scriptObject ) {
		sdScriptHelper h1;
		h1.Push( vehicle->GetScriptObject() );
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnCockpitSetup" ), h1 );
	}
}

/*
===================
idPlayerView::ClearCockpit
===================
*/
void idPlayerView::ClearCockpit( void ) {
	if ( cockpit.IsValid() ) {
		cockpit->Dispose();
		cockpit = NULL;
	}
}

/*
===================
idPlayerView::BuildViewFrustum
===================
*/
void idPlayerView::BuildViewFrustum( const renderView_t* view, idFrustum& viewFrustum ) {
	float dNear, dFar, dLeft, dUp;

	dNear = 0.0f;
	dFar = MAX_WORLD_SIZE;
	dLeft = dFar * idMath::Tan( DEG2RAD( view->fov_x * 0.5f ) );
	dUp = dFar * idMath::Tan( DEG2RAD( view->fov_y * 0.5f ) );
	viewFrustum.SetOrigin( view->vieworg );
	viewFrustum.SetAxis( view->viewaxis );
	viewFrustum.SetSize( dNear, dFar, dLeft, dUp );
}

/*
===================
idPlayerView::DrawWorldGuis
===================
*/
void idPlayerView::DrawWorldGuis( const renderView_t* view, const idFrustum& viewFrustum ) {
	pvsHandle_t handle = gameLocal.pvs.SetupCurrentPVS( view->vieworg, PVS_ALL_PORTALS_OPEN );

	sdInstanceCollector< sdGuiSurface > guiSurfaces( false );
	for ( int i = 0; i < guiSurfaces.Num(); i++ ) {
		guiSurfaces[i]->DrawCulled( handle, viewFrustum );
	}

	gameLocal.pvs.FreeCurrentPVS( handle );
}

/*
===================
idPlayerView::UpdateOcclusionQueries
===================
*/
void idPlayerView::UpdateOcclusionQueries( const renderView_t* view ) {
	const idList< idEntity* > &ocl = gameLocal.GetOcclusionQueryList();

	for ( int i = 0; i < ocl.Num(); i++ ) {
		if ( ocl[i] == NULL ) {
			continue;
		}

		int handle = ocl[i]->GetOcclusionQueryHandle();
		if ( handle == -1 ) {
			continue;
		}

		const occlusionTest_t& occDef = ocl[i]->UpdateOcclusionInfo( view->viewID );
		gameRenderWorld->UpdateOcclusionTestDef( handle, &occDef );
	}
}

idCVar g_spectateViewLerpScale( "g_spectateViewLerpScale", "0.7", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE, "Controls view smoothing for spectators", 0.2f, 1.f );

/*
===================
idPlayerView::UpdateSpectateView
===================
*/
void idPlayerView::UpdateSpectateView( idPlayer* other ) {
	const float lerpRate = g_spectateViewLerpScale.GetFloat();

	if ( other != NULL ) {
		if ( gameLocal.time > lastSpectateUpdateTime ) {
			lastSpectateUpdateTime = gameLocal.time;
			
			// this is a spectator view
			if ( lastSpectatePlayer != other ) {
				lastSpectatePlayer = other;
				lastSpectateOrigin = other->firstPersonViewOrigin;
				lastSpectateAxis = other->firstPersonViewAxis.ToQuat();
			} else {
				idVec3 diff = other->firstPersonViewOrigin - lastSpectateOrigin;
				if ( diff.LengthSqr() < Square( 1024.f ) ) {
					diff *= lerpRate;
					other->firstPersonViewOrigin = lastSpectateOrigin + diff;
				}
				lastSpectateOrigin = other->firstPersonViewOrigin;
				lastSpectateAxis = other->firstPersonViewAxis.ToQuat();
			}
		} else {
			other->firstPersonViewOrigin = lastSpectateOrigin;
			other->firstPersonViewAxis = lastSpectateAxis.ToMat3();
		}
	} else {
		lastSpectatePlayer = NULL;
	}
}

/*
===================
idPlayerView::Init
===================
*/
void idPlayerView::Init( void ) {
	lastSpectateUpdateTime = 0;
}

void idPlayerView::SetupEffect( void ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	
	renderEffect_t &renderEffect = underWaterEffect.GetRenderEffect();
	renderEffect.declEffect = gameLocal.FindEffect( localPlayer->spawnArgs.GetString( "fx_underWater" ), false );
	renderEffect.axis.Identity();
	renderEffect.loop = true;
	renderEffect.shaderParms[SHADERPARM_RED]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	underWaterEffectRunning = false;
}
