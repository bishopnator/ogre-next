#include "CustomHlms.h"
#include "GraphicsSystem.h"
#include "GraphicsGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreArchiveManager.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

// Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#    else
int mainApp( int argc, const char *argv[] )
#    endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded(DEMO_MAIN_ENTRY_PARAMS);
}
#endif

namespace Demo
{
    class CustomHlmsGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "CustomHlmsWorkspace", true );
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( dataFolder.end() - 1 ) != '/' )
                dataFolder += "/";

            addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
        }

        void registerHlms() override
        {
            GraphicsSystem::registerHlms();

            // Register CustomHlms to OGRE.
            Ogre::registerHlms<CustomHlms>(mResourcePath, getMediaReadArchiveType());
        }

    public:
        CustomHlmsGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, LogicSystem **outLogicSystem )
    {
        GraphicsGameState *gfxGameState = new GraphicsGameState(
            "Shows how to use the custom HLMS.\n"
            "\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/Hlms/Common/*.*\n"
            "   * Samples/Media/Hlms/CustomHlms/*.*\n"
            "   * Samples/Media/2.0/scripts/Compositors/CustomHlms.compositor\n"
        );

        GraphicsSystem *graphicsSystem = new CustomHlmsGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState, GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState, LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char *MainEntryPoints::getWindowTitle()
    {
        return "Custom HLMS Tutorial";
    }
}  // namespace Demo
