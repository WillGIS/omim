#import "DownloadIndicatorProtocol.h"
#import "MWMMyTarget.h"
#import "MWMNavigationController.h"

@class MapViewController;

@interface MapsAppDelegate
    : UIResponder<UIApplicationDelegate, UIAlertViewDelegate, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
  UIBackgroundTaskIdentifier m_editorUploadBackgroundTask;
  UIAlertView * m_loadingAlertView;
}

@property(nonatomic) UIWindow * window;

@property(nonatomic, readonly) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

@property(nonatomic) MWMMyTarget * myTarget;

+ (MapsAppDelegate *)theApp;

- (BOOL)hasApiURL;

- (void)enableStandby;
- (void)disableStandby;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)showMap;
- (void)showAlertIfRequired;

@end
