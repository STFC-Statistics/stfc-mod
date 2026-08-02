# IL2CPP Drift Report

Generated: 2026-08-02T01:12:47.7771423+02:00

This is a static lookup preflight. It does not prove native detour safety, signatures, offsets, or runtime behavior.

| Metric | Count |
|---|---:|
| Class lookups present in both builds | 164 |
| Class lookups missing in both builds | 58 |
| Manual-review results | 451 |

## Results

| Kind | Assembly | Namespace | Class | Member | Source | Line | Status | Evidence |
|---|---|---|---|---|---|---:|---|---|
| class | Assembly-CSharp |  | DeploymentManager |  | mods\src\prime\DeploymentManager.h | 96 | present-both | static-confirmed |
| class | Assembly-CSharp |  | FleetsManager.<Tow>d__192 |  | mods\src\prime\FleetsManager.h | 42 | missing-both | manual-review |
| class | Assembly-CSharp |  | MonoSingleton |  | mods\src\patches\parts\transition_screen.cc | 351 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | AppConfig |  | mods\src\patches\parts\testing.cc | 39 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | Hub |  | mods\src\prime\Hub.h | 298 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | Model |  | mods\src\patches\parts\testing.cc | 89 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | Model |  | mods\src\patches\parts\testing.cc | 169 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | PrimeApp |  | mods\src\patches\parts\sync.cc | 2244 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | PrimeApp |  | mods\src\prime\Hub.h | 263 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Core | SortingPredicates |  | mods\src\patches\parts\officer_sort.cc | 124 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Localization | LanguageManager |  | mods\src\patches\notification_service.cc | 422 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Localization | LanguageManager |  | mods\src\prime\LanguageManager.h | 23 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Localization | LocaleUtilities |  | mods\src\patches\battle_notify_parser.cc | 71 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Localization | LocaleUtilities |  | mods\src\patches\notification_service.cc | 444 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Sections | SectionManager |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 352 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Sections | SectionManager |  | mods\src\prime\Hub.h | 220 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Sections | SectionNavHistory |  | mods\src\prime\Hub.h | 190 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Sections | SectionStorage |  | mods\src\prime\Hub.h | 165 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Sorting | SortComparer |  | mods\src\patches\parts\officer_sort.cc | 137 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | BaseListContainer |  | mods\src\prime\TabBarViewController.h | 15 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | CanvasController |  | mods\src\patches\parts\ui_scale.cc | 92 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | CanvasController |  | mods\src\prime\CanvasController.h | 68 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | CanvasController |  | mods\src\prime\CanvasController.h | 85 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | ColourTextLocalizer |  | mods\src\patches\parts\cargo_format.cc | 69 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | GenericButtonContext |  | mods\src\prime\GenericButtonContext.h | 13 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | GenericButtonWidget |  | mods\src\prime\GenericButtonWidget.h | 15 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | InputFieldWidget |  | mods\src\prime\InputFieldWidget.h | 28 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | LocaleTextContext |  | mods\src\patches\battle_notify_parser.cc | 62 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | ScreenManager |  | mods\src\patches\parts\hotkeys.cc | 919 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | ScreenManager |  | mods\src\patches\parts\ui_scale.cc | 80 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | ScreenManager |  | mods\src\prime\ScreenManager.h | 22 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | SemaphoreButtonListener |  | mods\src\prime\SemaphoreButtonListener.h | 16 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | SemaphoreListenerBase |  | mods\src\prime\SemaphoreListenerBase.h | 13 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | SwipeScroller |  | mods\src\prime\ChatPreviewController.h | 28 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | TabBar |  | mods\src\prime\TabBarViewController.h | 37 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | TabBarViewController |  | mods\src\prime\TabBarViewController.h | 69 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | TextLocalizer |  | mods\src\patches\parts\cargo_format.cc | 28 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.UI | VisibilityController |  | mods\src\prime\VisibilityController.h | 46 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Utils | AspectRatioConstraintHandler |  | mods\src\patches\parts\free_resize.cc | 165 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Client.Utils | AspectRatioConstraintHandler |  | mods\src\prime\AspectRatioConstraintHandler.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Bookmarks | BookmarksManager |  | mods\src\prime\BookmarksManager.h | 83 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Bookmarks | CoordinateSearchContext |  | mods\src\prime\BookmarksManager.h | 42 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | ChatManager |  | mods\src\prime\ChatManager.h | 89 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | ChatMessageListLocalViewController |  | mods\src\prime\ChatMessageListLocalViewController.h | 41 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | ChatPreviewController |  | mods\src\patches\parts\chat.cc | 163 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | ChatPreviewController |  | mods\src\prime\ChatPreviewController.h | 48 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | ChatSectionContext |  | mods\src\prime\ChatMessageListLocalViewController.h | 16 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | FullScreenChatViewController |  | mods\src\patches\parts\chat.cc | 146 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Chat | FullScreenChatViewController |  | mods\src\prime\FullScreenChatViewController.h | 20 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | PreScanStationTargetWidget |  | mods\src\prime\PreScanStationTargetWidget.h | 17 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | PreScanTargetWidget |  | mods\src\patches\parts\hotkeys.cc | 946 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | PreScanTargetWidget |  | mods\src\prime\PreScanTargetWidget.h | 45 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | RewardsButtonWidget |  | mods\src\patches\parts\hotkeys.cc | 932 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | RewardsButtonWidget |  | mods\src\prime\RewardsButtonWidget.h | 24 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | ScanEngageButtonsWidget |  | mods\src\prime\RequestDispatcherBase.h | 13 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | ScanEngageButtonsWidget |  | mods\src\prime\ScanEngageButtonsWidget.h | 38 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Combat | ScanTargetViewController |  | mods\src\prime\ScanTargetViewController.h | 20 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.FleetManagement | FleetsManager |  | mods\src\prime\FleetsManager.h | 117 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.GameInput | ShortcutsManager |  | mods\src\patches\parts\hotkeys.cc | 907 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.HUD | FleetBarContext |  | mods\src\prime\FleetBarViewController.h | 14 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.HUD | FleetBarViewController |  | mods\src\prime\FleetBarViewController.h | 80 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.HUD | Toast |  | mods\src\prime\Toast.h | 93 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.HUD | ToastObserver |  | mods\src\patches\parts\disable_banners.cc | 49 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Interstitial | InterstitialViewController |  | mods\src\patches\parts\misc.cc | 308 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Interstitial | InterstitialViewController |  | mods\src\prime\InterstitialViewController.h | 12 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Inventories | InventoryForPopup |  | mods\src\patches\parts\misc.cc | 54 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Inventories | InventoryForPopup |  | mods\src\prime\InventoryForPopup.h | 13 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Inventories | InventoryListViewController |  | mods\src\prime\InventoryListViewController.h | 23 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.LoadingScreen | BlurController |  | mods\src\prime\BlurController.h | 14 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.LoadingScreen | TransitionManager |  | mods\src\prime\TransitionManager.h | 14 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.LoadingScreen | TransitionViewController |  | mods\src\patches\parts\transition_screen.cc | 42 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.LoadingScreen | TransitionViewController |  | mods\src\patches\parts\transition_screen.cc | 126 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.LoadingScreen | TransitionViewController |  | mods\src\patches\parts\transition_screen.cc | 315 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.LoadingScreen | TransitionViewController |  | mods\src\prime\TransitionViewController.h | 15 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Login | LoginSequence |  | mods\src\patches\parts\loading_screen.cc | 56 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Login | LoginSequence |  | mods\src\patches\parts\loading_screen.cc | 104 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Missions.UI | AnimatedRewardsScreenViewController |  | mods\src\prime\AnimatedRewardsScreenViewController.h | 12 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationInteractionUIContext |  | mods\src\prime\NavigationInteractionUIContext.h | 11 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationInteractionUIViewController |  | mods\src\prime\NavigationInteractionUIViewController.h | 17 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationManager |  | mods\src\prime\NavigationDirector.h | 16 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationManager |  | mods\src\prime\NavigationManager.h | 15 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationPan |  | mods\src\patches\parts\fix_pan.cc | 63 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationPan |  | mods\src\prime\NavigationPan.h | 39 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationSectionManager |  | mods\src\prime\NavigationSectionManager.h | 29 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationZoom |  | mods\src\patches\parts\zoom.cc | 341 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | NavigationZoom |  | mods\src\prime\NavigationZoom.h | 60 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | PlanetViewUtils |  | mods\src\patches\parts\zoom.cc | 321 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | PlanetViewUtils |  | mods\src\prime\PlanetViewUtils.h | 22 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | PopulatedSystemData |  | mods\src\prime\PopulatedSystemData.h | 14 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Navigation | SystemViewScaler |  | mods\src\prime\SystemViewScaler.h | 12 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | AllianceStarbaseObjectViewerWidget |  | mods\src\prime\AllianceStarbaseObjectViewerWidget.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | ArmadaObjectViewerWidget |  | mods\src\prime\ArmadaObjectViewerWidget.h | 71 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | CelestialObjectViewerWidget |  | mods\src\prime\CelestialObjectViewerWidget.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | EmbassyObjectViewer |  | mods\src\prime\EmbassyObjectViewer.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | HousingObjectViewerWidget |  | mods\src\prime\HousingObjectViewerWidget.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | MiningObjectViewerWidget |  | mods\src\prime\MiningObjectViewerWidget.h | 35 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | MissionsObjectViewerWidget |  | mods\src\prime\MissionsObjectViewerWidget.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | ParentObjectViewerViewController |  | mods\src\prime\ParentObjectViewerViewController.h | 20 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.ObjectViewer | StarNodeObjectViewerWidget |  | mods\src\prime\StarNodeObjectViewerWidget.h | 24 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.OfficerAssignment | OfficerAssignmentViewController |  | mods\src\prime\OfficerAssignmentViewController.h | 21 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Officers | OfficerSortGenerators |  | mods\src\patches\parts\officer_sort.cc | 160 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.SharedFeatures | GenericRewardsScreenViewController |  | mods\src\prime\GenericRewardsScreenViewController.h | 10 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Ships | AssignShipsWidget |  | mods\src\prime\AssignShipsWidget.h | 18 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Ships | FleetLocalViewController |  | mods\src\prime\FleetLocalViewController.h | 56 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | BundleDataWidget |  | mods\src\patches\parts\misc.cc | 67 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | BundleDataWidget |  | mods\src\prime\BundleDataWidget.h | 63 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | BundleGroupConfig |  | mods\src\patches\parts\misc.cc | 221 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | BundleGroupConfig |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 112 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | ShopListScrollerViewController |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 151 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | ShopListViewController |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 329 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | ShopSceneManager |  | mods\src\patches\parts\misc.cc | 295 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | ShopSectionContext |  | mods\src\patches\parts\misc.cc | 247 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | ShopSectionContext |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 134 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.Shop | ShopSummaryDirector |  | mods\src\prime\ShopSummaryDirector.h | 17 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.SlideShow | SlideShowViewController |  | mods\src\patches\parts\transition_screen.cc | 296 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.SlideShow | SlideShowViewController |  | mods\src\patches\parts\transition_screen.cc | 344 | present-both | static-confirmed |
| class | Assembly-CSharp | Digit.Prime.UI | ElementSelectorViewController |  | mods\src\prime\ElementSelectorViewController.h | 13 | present-both | static-confirmed |
| class | Assembly-CSharp | Prime.ActionQueue | ActionQueueManager |  | mods\src\patches\parts\misc.cc | 321 | present-both | static-confirmed |
| class | Assembly-CSharp | Prime.ActionQueue | ActionQueueManager |  | mods\src\patches\parts\testing.cc | 196 | present-both | static-confirmed |
| class | Assembly-CSharp | Prime.ActionQueue | ActionQueueManager |  | mods\src\prime\ActionQueueManager.h | 100 | present-both | static-confirmed |
| class | Assembly-CSharp | Prime.LoadingScreen | LoadingTipViewController |  | mods\src\patches\parts\loading_tip.cc | 141 | present-both | static-confirmed |
| class | Assembly-CSharp | Prime.SharedFeatures.Scripts.UI.Widgets | DrawerContext |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 28 | present-both | static-confirmed |
| class | Assembly-CSharp | Prime.SharedFeatures.Scripts.UI.Widgets | DrawerWidget |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 51 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime |  | DeploymentService.<PlanCourse>d__119 |  | mods\src\prime\DeploymentManager.h | 36 | missing-both | manual-review |
| class | Digit.Client.PrimeLib.Runtime |  | FleetPlayerData.CanRepairRequirement |  | mods\src\prime\CanRepairRequirement.h | 15 | missing-both | manual-review |
| class | Digit.Client.PrimeLib.Runtime |  | FleetPlayerData.RecallRequirement |  | mods\src\prime\RecallRequirement.h | 15 | missing-both | manual-review |
| class | Digit.Client.PrimeLib.Runtime | Digit | IBuffData |  | mods\src\prime\IBuffData.h | 15 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.Client.Core | MathUtils |  | mods\src\patches\parts\zoom.cc | 18 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.Client.Core | SortFunction |  | mods\src\patches\parts\officer_sort.cc | 131 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.Networking.Core | GSServiceRegistry |  | mods\src\prime\Hub.h | 248 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.Networking.RTC | RealtimeDataPayload |  | mods\src\prime\RealtimeDataPayload.h | 28 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.Prime.Shop | ShopCategory |  | mods\src\patches\parts\misc.cc | 180 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.Prime.Shop | ShopCategory |  | mods\src\patches\parts\open_bulk_claim_gifts.cc | 89 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimePlatform.Content | Bundle |  | mods\src\prime\BundleDataWidget.h | 16 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimePlatform.Content | CurrencyType |  | mods\src\patches\parts\misc.cc | 201 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimePlatform.Core | PlatformModelRegistry |  | mods\src\patches\parts\sync.cc | 2112 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Core | GameServer |  | mods\src\patches\parts\sync.cc | 2256 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Core | GameServerModelRegistry |  | mods\src\patches\parts\sync.cc | 2092 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | BattleResultHeader |  | mods\src\prime\BattleResultHeader.h | 69 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | BattleTargetData |  | mods\src\patches\parts\testing.cc | 183 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | BattleTargetData |  | mods\src\prime\BattleTargetData.h | 14 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | EntityGroup |  | mods\src\prime\EntityGroup.h | 229 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | FleetDeployedData |  | mods\src\prime\FleetDeployedData.h | 25 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | FleetPlayerData |  | mods\src\prime\FleetPlayerData.h | 45 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | HullSpec |  | mods\src\prime\HullSpec.h | 25 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | MissionsDataContainer |  | mods\src\patches\parts\sync.cc | 2191 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | ServiceResponse |  | mods\src\prime\ServiceResponse.h | 42 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | StateContainer`1 |  | mods\src\prime\StateContainer.h | 19 | missing-both | manual-review |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Models | UserProfile |  | mods\src\prime\UserProfile.h | 14 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Parsers | SlotAssignRtcParser |  | mods\src\patches\parts\sync.cc | 2279 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Parsers | SlotClearRtcParser |  | mods\src\patches\parts\sync.cc | 2290 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | BuffDataContainer |  | mods\src\patches\parts\sync.cc | 2125 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | BuffService |  | mods\src\patches\parts\buff_fixes.cc | 30 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | BuffService |  | mods\src\patches\parts\misc.cc | 283 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | BuffService |  | mods\src\patches\parts\sync.cc | 2142 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | DeploymentService |  | mods\src\prime\DeploymentManager.h | 47 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | IBuffComparer |  | mods\src\prime\IBuffComparer.h | 24 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | JobService |  | mods\src\patches\parts\sync.cc | 2167 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | ResearchService |  | mods\src\patches\parts\sync.cc | 2215 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | SlotDataContainer |  | mods\src\patches\parts\sync.cc | 2227 | present-both | static-confirmed |
| class | Digit.Client.PrimeLib.Runtime | Digit.PrimeServer.Services | SpecService |  | mods\src\prime\SpecService.h | 19 | present-both | static-confirmed |
| class | Digit.Engine.HttpClient.Runtime | Digit.Engine.HttpClient | HttpJob |  | mods\src\prime\HttpJob.h | 29 | present-both | static-confirmed |
| class | Digit.Engine.HttpClient.Runtime | Digit.Engine.HttpClient | HttpRequest |  | mods\src\prime\HttpRequest.h | 18 | present-both | static-confirmed |
| class | Digit.Engine.HttpClient.Runtime | Digit.Engine.HttpClient | HttpResponse |  | mods\src\prime\HttpResponse.h | 29 | present-both | static-confirmed |
| class | Digit.Engine.Utilities.Runtime | Digit.Networking.Core | CallbackContainer`1 |  | mods\src\prime\CallbackContainer.h | 87 | missing-both | manual-review |
| class | Google.Protobuf | Google.Protobuf | ByteString |  | mods\src\prime\EntityGroup.h | 22 | present-both | static-confirmed |
| class | Google.Protobuf | Google.Protobuf.Collections | RepeatedField`1 |  | mods\src\prime\ServiceResponse.h | 22 | missing-both | manual-review |
| class | mscorlib | System | Byte |  | mods\src\patches\parts\loading_screen_common.h | 203 | missing-both | manual-review |
| class | mscorlib | System | Int64 |  | mods\src\patches\battle_notify_parser.cc | 92 | missing-both | manual-review |
| class | mscorlib | System | Object |  | mods\src\patches\battle_notify_parser.cc | 88 | missing-both | manual-review |
| class | mscorlib | System | Object |  | mods\src\patches\notification_service.cc | 465 | missing-both | manual-review |
| class | TouchKit |  | TKTouch |  | mods\src\patches\parts\fix_pan.cc | 53 | present-both | static-confirmed |
| class | TouchKit |  | TKTouch |  | mods\src\prime\TKTouch.h | 20 | present-both | static-confirmed |
| class | Unity.TextMeshPro | TMPro | TMP_InputField |  | mods\src\prime\TMP_InputField.h | 39 | present-both | static-confirmed |
| class | Unity.TextMeshPro | TMPro | TMP_Text |  | mods\src\patches\parts\loading_tip.cc | 51 | present-both | static-confirmed |
| class | UnityEngine.CoreModule | UnityEngine | Behaviour |  | mods\src\patches\parts\transition_screen.cc | 128 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Behaviour |  | mods\src\patches\parts\transition_screen.cc | 275 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Camera |  | mods\src\prime\Camera.h | 16 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Component |  | mods\src\patches\parts\loading_screen.cc | 63 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Component |  | mods\src\patches\parts\transition_screen.cc | 119 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Component |  | mods\src\patches\parts\zoom.cc | 112 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Component |  | mods\src\prime\CanvasController.h | 79 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Cursor |  | mods\src\patches\parts\testing.cc | 157 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | GameObject |  | mods\src\patches\parts\loading_screen.cc | 70 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | GameObject |  | mods\src\patches\parts\loading_screen_common.h | 173 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | GameObject |  | mods\src\patches\parts\loading_screen_common.h | 419 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | GameObject |  | mods\src\patches\parts\transition_screen.cc | 44 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | GameObject |  | mods\src\patches\parts\transition_screen.cc | 193 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | GameObject |  | mods\src\prime\GameObject.h | 56 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | MonoBehaviour |  | mods\src\patches\parts\transition_screen.cc | 43 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | MonoBehaviour |  | mods\src\patches\parts\transition_screen.cc | 192 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Object |  | mods\src\patches\parts\loading_screen_common.h | 151 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | RectTransform |  | mods\src\patches\parts\loading_screen_common.h | 76 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | RectTransform |  | mods\src\patches\parts\loading_screen_common.h | 420 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | RectTransform |  | mods\src\patches\parts\transition_screen.cc | 195 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Screen |  | mods\src\patches\parts\loading_screen_common.h | 511 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Sprite |  | mods\src\patches\parts\loading_screen_common.h | 98 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Texture2D |  | mods\src\patches\parts\loading_screen_common.h | 196 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\patches\parts\loading_screen.cc | 69 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\patches\parts\loading_screen_common.h | 174 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\patches\parts\loading_screen_common.h | 421 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\patches\parts\transition_screen.cc | 45 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\patches\parts\transition_screen.cc | 168 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\patches\parts\transition_screen.cc | 194 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Transform |  | mods\src\prime\Transform.h | 26 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Vector2 |  | mods\src\prime\Vector2.h | 15 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine | Vector3 |  | mods\src\prime\Vector3.h | 14 | missing-both | manual-review |
| class | UnityEngine.CoreModule | UnityEngine.SceneManagement | SceneManager |  | mods\src\prime\SceneManager.h | 28 | missing-both | manual-review |
| class | UnityEngine.ImageConversionModule | UnityEngine | ImageConversion |  | mods\src\patches\parts\loading_screen_common.h | 197 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.EventSystems | EventSystem |  | mods\src\prime\EventSystem.h | 33 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.EventSystems | UIBehaviour |  | mods\src\prime\UIBehaviour.h | 9 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Button |  | mods\src\prime\Button.h | 17 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | CanvasScaler |  | mods\src\prime\CanvasScaler.h | 26 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Image |  | mods\src\patches\parts\loading_screen.cc | 71 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Image |  | mods\src\patches\parts\loading_screen_common.h | 110 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Image |  | mods\src\patches\parts\loading_screen_common.h | 140 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Image |  | mods\src\patches\parts\loading_screen_common.h | 422 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Image |  | mods\src\patches\parts\loading_screen_common.h | 558 | missing-both | manual-review |
| class | UnityEngine.UI | UnityEngine.UI | Image |  | mods\src\patches\parts\transition_screen.cc | 46 | missing-both | manual-review |
| class | UnityEngine.UIModule | UnityEngine | Canvas |  | mods\src\patches\parts\loading_screen_common.h | 175 | missing-both | manual-review |
| class | UnityEngine.UIModule | UnityEngine | Canvas |  | mods\src\prime\Canvas.h | 12 | missing-both | manual-review |
| class | UnityEngine.UnityWebRequestModule | UnityEngine.Networking | UnityWebRequest |  | mods\src\prime\UnityWebRequest.h | 18 | missing-both | manual-review |
| manual-review |  |  |  | dynamic lookup | mods\src\il2cpp\il2cpp_helper.h | 156 | manual-review | manual-review |
| manual-review |  |  |  | dynamic lookup | mods\src\il2cpp\il2cpp_helper.h | 265 | manual-review | manual-review |
| manual-review |  |  |  | dynamic lookup | mods\src\il2cpp\il2cpp_helper.h | 274 | manual-review | manual-review |
| manual-review |  |  |  | dynamic lookup | mods\src\il2cpp\il2cpp_helper.h | 279 | manual-review | manual-review |
| member |  |  | scope-ambiguous | .ctor | mods\src\patches\battle_notify_parser.cc | 65 | manual-review | manual-review |
| member |  |  | scope-ambiguous | .ctor | mods\src\patches\parts\object_tracker.cc | 174 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _activeSystemParent | mods\src\prime\PlanetViewUtils.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _actualDistance | mods\src\prime\NavigationZoom.h | 139 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _actualDistance | mods\src\prime\NavigationZoom.h | 145 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _addToQueueButtonWidget | mods\src\prime\PreScanTargetWidget.h | 58 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _animator | mods\src\patches\parts\transition_screen.cc | 127 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _animator | mods\src\prime\TransitionViewController.h | 30 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _appConfig | mods\src\patches\parts\testing.cc | 96 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _armadaAttackButton | mods\src\prime\PreScanTargetWidget.h | 64 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _armadaButton | mods\src\prime\RequestDispatcherBase.h | 20 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _armadaButton | mods\src\prime\ScanEngageButtonsWidget.h | 45 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _ascending | mods\src\patches\parts\officer_sort.cc | 196 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _assignmentOptions | mods\src\patches\parts\officer_sort.cc | 206 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _backdrop | mods\src\prime\PopulatedSystemData.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _backLogicSkipSectionIds | mods\src\prime\ShopSummaryDirector.h | 24 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _baseSystemRadius | mods\src\prime\SystemViewScaler.h | 19 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _baseSystemRadius | mods\src\prime\SystemViewScaler.h | 25 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _battleTargetData | mods\src\prime\PreScanTargetWidget.h | 52 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _blurTime | mods\src\prime\BlurController.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _blurTime | mods\src\prime\BlurController.h | 26 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _button | mods\src\prime\SemaphoreButtonListener.h | 22 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _categoriesTabBarViewController | mods\src\prime\FullScreenChatViewController.h | 32 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _category | mods\src\patches\parts\misc.cc | 228 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _category | mods\src\patches\parts\open_bulk_claim_gifts.cc | 124 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _category | mods\src\patches\parts\open_bulk_claim_gifts.cc | 316 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _category | mods\src\prime\ChatMessageListLocalViewController.h | 23 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _currency | mods\src\patches\parts\misc.cc | 234 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _currentContentIndex | mods\src\prime\ChatPreviewController.h | 34 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _currentState | mods\src\prime\BundleDataWidget.h | 70 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _data | mods\src\prime\TabBarViewController.h | 22 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _data | mods\src\prime\TabBarViewController.h | 56 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _defaultOptions | mods\src\prime\TabBarViewController.h | 44 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _deltaScaler | mods\src\prime\NavigationPan.h | 58 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _depth | mods\src\prime\NavigationPan.h | 93 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _depth | mods\src\prime\NavigationZoom.h | 79 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _depth | mods\src\prime\NavigationZoom.h | 85 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _descending | mods\src\patches\parts\officer_sort.cc | 197 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _displayKey | mods\src\patches\parts\officer_sort.cc | 195 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _displayKey | mods\src\patches\parts\officer_sort.cc | 213 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _displayKey | mods\src\patches\parts\officer_sort.cc | 355 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farMagRadiusRatioSystemExtended | mods\src\prime\NavigationPan.h | 76 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farMagRadiusRatioSystemExtended | mods\src\prime\NavigationPan.h | 82 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farMagRadiusRatioSystemNormal | mods\src\prime\NavigationPan.h | 64 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farMagRadiusRatioSystemNormal | mods\src\prime\NavigationPan.h | 70 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farRatioSystemExtended | mods\src\prime\NavigationZoom.h | 241 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farRatioSystemExtended | mods\src\prime\NavigationZoom.h | 247 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farRatioSystemNormal | mods\src\prime\NavigationZoom.h | 229 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _farRatioSystemNormal | mods\src\prime\NavigationZoom.h | 235 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _fleetPanelController | mods\src\prime\FleetBarViewController.h | 87 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _focusedPanel | mods\src\prime\ChatPreviewController.h | 66 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _focusedPanel | mods\src\prime\ChatPreviewController.h | 72 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _history | mods\src\prime\Hub.h | 239 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _image | mods\src\patches\parts\transition_screen.cc | 297 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _input | mods\src\prime\InputFieldWidget.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _inputField | mods\src\prime\AssignShipsWidget.h | 24 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _inputField | mods\src\prime\ChatMessageListLocalViewController.h | 47 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _inputField | mods\src\prime\InventoryListViewController.h | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _inputField | mods\src\prime\OfficerAssignmentViewController.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _inputFieldSelected | mods\src\prime\ChatMessageListLocalViewController.h | 52 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _joinContext | mods\src\prime\ArmadaObjectViewerWidget.h | 78 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _lastDelta | mods\src\prime\NavigationPan.h | 52 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _lastZoomDelta | mods\src\prime\NavigationZoom.h | 187 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _lastZoomDelta | mods\src\prime\NavigationZoom.h | 193 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _listContainer | mods\src\prime\TabBarViewController.h | 50 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _locaId | mods\src\prime\UserProfile.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _mainCanvas | mods\src\patches\parts\loading_screen.cc | 58 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _maximum | mods\src\prime\NavigationZoom.h | 127 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _maximum | mods\src\prime\NavigationZoom.h | 133 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _maximumCanvasScaleFactor | mods\src\prime\ScreenManager.h | 41 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _messageList | mods\src\prime\FullScreenChatViewController.h | 26 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _minimum | mods\src\prime\NavigationZoom.h | 115 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _minimum | mods\src\prime\NavigationZoom.h | 121 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _minimumCanvasScaleFactor | mods\src\prime\ScreenManager.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _nearMagRadiusRatioSystemNormal | mods\src\prime\NavigationPan.h | 88 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _occupiedState | mods\src\prime\MiningObjectViewerWidget.h | 42 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _panelIndicators | mods\src\prime\ChatPreviewController.h | 54 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _popData | mods\src\prime\PlanetViewUtils.h | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _rewardsButtonWidget | mods\src\prime\PreScanTargetWidget.h | 76 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _rewardsController | mods\src\prime\RewardsButtonWidget.h | 31 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _rosterOptions | mods\src\patches\parts\officer_sort.cc | 205 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _rosterSorters | mods\src\patches\parts\officer_sort.cc | 204 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _scanEngageButtonsWidget | mods\src\prime\MiningObjectViewerWidget.h | 48 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _scanEngageButtonsWidget | mods\src\prime\PreScanTargetWidget.h | 70 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _scanEngageButtonsWidget | mods\src\prime\ScanTargetViewController.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _sceneCamera | mods\src\prime\NavigationZoom.h | 223 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _scroller | mods\src\prime\TransitionViewController.h | 36 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _sectionStorage | mods\src\prime\Hub.h | 233 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _selectionDrawerWidget | mods\src\patches\parts\open_bulk_claim_gifts.cc | 164 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _selectionDrawerWidget | mods\src\patches\parts\open_bulk_claim_gifts.cc | 342 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _semaphoreButtonListener | mods\src\prime\GenericButtonWidget.h | 26 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _shipBarItemLocalViewController | mods\src\prime\FleetLocalViewController.h | 69 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _size | mods\src\prime\IList.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _sortingOption | mods\src\patches\parts\officer_sort.cc | 214 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _sortingOption | mods\src\patches\parts\officer_sort.cc | 241 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _state | mods\src\prime\VisibilityController.h | 53 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _staticOverride | mods\src\patches\parts\transition_screen.cc | 50 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _staticOverride | mods\src\prime\TransitionViewController.h | 23 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _storedZoomDistanceSystem | mods\src\prime\NavigationZoom.h | 151 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _storedZoomDistanceSystem | mods\src\prime\NavigationZoom.h | 157 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _swipeScroller | mods\src\prime\ChatPreviewController.h | 60 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _systemDefaultZoomRatio | mods\src\prime\NavigationZoom.h | 163 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _systemDefaultZoomRatio | mods\src\prime\NavigationZoom.h | 169 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _systemId | mods\src\prime\PopulatedSystemData.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _tabBar | mods\src\prime\TabBarViewController.h | 76 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _targetFleetData | mods\src\prime\FleetsManager.h | 109 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _targetSection | mods\src\prime\InventoryListViewController.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _trackingPOI | mods\src\prime\NavigationPan.h | 46 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _viewRadius | mods\src\prime\NavigationZoom.h | 91 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _viewRadius | mods\src\prime\NavigationZoom.h | 97 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _visibilityController | mods\src\prime\ObjectViewerBaseWidget.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _visibilityController | mods\src\prime\ParentObjectViewerViewController.h | 50 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _waitFrames | mods\src\prime\BlurController.h | 32 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _waitFrames | mods\src\prime\BlurController.h | 37 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _waitFrames | mods\src\prime\TransitionManager.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _waitFrames | mods\src\prime\TransitionManager.h | 32 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _worldPoint | mods\src\prime\NavigationZoom.h | 211 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _worldPoint | mods\src\prime\NavigationZoom.h | 217 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _zoomDelta | mods\src\prime\NavigationZoom.h | 103 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _zoomDelta | mods\src\prime\NavigationZoom.h | 109 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _zoomLocation | mods\src\prime\NavigationZoom.h | 199 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _zoomLocation | mods\src\prime\NavigationZoom.h | 205 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _zoomtotal | mods\src\prime\NavigationZoom.h | 175 | manual-review | manual-review |
| member |  |  | scope-ambiguous | _zoomtotal | mods\src\prime\NavigationZoom.h | 181 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AboutToHide | mods\src\patches\parts\open_bulk_claim_gifts.cc | 332 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AboutToHide | mods\src\patches\parts\transition_screen.cc | 337 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AboutToShow | mods\src\patches\parts\chat.cc | 150 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AboutToShow | mods\src\patches\parts\chat.cc | 167 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AboutToShow | mods\src\patches\parts\misc.cc | 312 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AboutToShow | mods\src\patches\parts\transition_screen.cc | 330 | manual-review | manual-review |
| member |  |  | scope-ambiguous | activeInHierarchy | mods\src\prime\GameObject.h | 63 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AddActionToQueue | mods\src\patches\parts\misc.cc | 325 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AddComponent | mods\src\patches\parts\loading_screen_common.h | 430 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Address | mods\src\prime\FleetPlayerData.h | 57 | manual-review | manual-review |
| member |  |  | scope-ambiguous | App | mods\src\prime\Hub.h | 277 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ApplyIdentifierParameters | mods\src\patches\battle_notify_parser.cc | 66 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AssetUrlOverride | mods\src\patches\parts\testing.cc | 70 | manual-review | manual-review |
| member |  |  | scope-ambiguous | AssetUrlOverride | mods\src\patches\parts\testing.cc | 76 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Awake | mods\src\patches\parts\loading_screen.cc | 110 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Awake | mods\src\patches\parts\transition_screen.cc | 323 | manual-review | manual-review |
| member |  |  | scope-ambiguous | backgroundColor | mods\src\prime\Camera.h | 56 | manual-review | manual-review |
| member |  |  | scope-ambiguous | backgroundColor | mods\src\prime\Camera.h | 62 | manual-review | manual-review |
| member |  |  | scope-ambiguous | BattleType | mods\src\prime\BattleResultHeader.h | 61 | manual-review | manual-review |
| member |  |  | scope-ambiguous | BlurController | mods\src\prime\TransitionManager.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Body | mods\src\prime\HttpResponse.h | 36 | manual-review | manual-review |
| member |  |  | scope-ambiguous | BundleGroup | mods\src\patches\parts\misc.cc | 254 | manual-review | manual-review |
| member |  |  | scope-ambiguous | bytes | mods\src\prime\EntityGroup.h | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CameraZoomedEventHandler | mods\src\patches\parts\zoom.cc | 323 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CanvasContext | mods\src\prime\ChatMessageListLocalViewController.h | 57 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CanvasContext | mods\src\prime\FleetBarViewController.h | 70 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CanvasContext | mods\src\prime\ViewController.h | 19 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ChannelId | mods\src\prime\RealtimeDataPayload.h | 41 | manual-review | manual-review |
| member |  |  | scope-ambiguous | clearFlags | mods\src\prime\Camera.h | 45 | manual-review | manual-review |
| member |  |  | scope-ambiguous | clearFlags | mods\src\prime\Camera.h | 50 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Context | mods\src\patches\parts\open_bulk_claim_gifts.cc | 64 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Context | mods\src\patches\parts\open_bulk_claim_gifts.cc | 298 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Context | mods\src\prime\Widget.h | 33 | manual-review | manual-review |
| member |  |  | scope-ambiguous | count | mods\src\prime\ServiceResponse.h | 30 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Create | mods\src\patches\parts\loading_screen_common.h | 99 | manual-review | manual-review |
| member |  |  | scope-ambiguous | current | mods\src\prime\EventSystem.h | 11 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CurrentFleet | mods\src\prime\FleetBarViewController.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CurrentSection | mods\src\prime\Hub.h | 227 | manual-review | manual-review |
| member |  |  | scope-ambiguous | currentSelectedGameObject | mods\src\prime\EventSystem.h | 40 | manual-review | manual-review |
| member |  |  | scope-ambiguous | currentSelectedGameObject | mods\src\prime\EventSystem.h | 46 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CurrentState | mods\src\prime\FleetPlayerData.h | 62 | manual-review | manual-review |
| member |  |  | scope-ambiguous | CurrentState | mods\src\prime\StateContainer.h | 24 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Data | mods\src\prime\RealtimeDataPayload.h | 65 | manual-review | manual-review |
| member |  |  | scope-ambiguous | DataType | mods\src\prime\RealtimeDataPayload.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Destroy | mods\src\patches\parts\loading_screen_common.h | 152 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Distance | mods\src\prime\NavigationZoom.h | 67 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Distance | mods\src\prime\NavigationZoom.h | 73 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Enabled | mods\src\patches\parts\open_bulk_claim_gifts.cc | 40 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Enabled | mods\src\patches\parts\open_bulk_claim_gifts.cc | 289 | manual-review | manual-review |
| member |  |  | scope-ambiguous | enabled | mods\src\prime\CanvasController.h | 37 | manual-review | manual-review |
| member |  |  | scope-ambiguous | enabled | mods\src\prime\Widget.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | EnemyShipHullId | mods\src\prime\BattleResultHeader.h | 82 | manual-review | manual-review |
| member |  |  | scope-ambiguous | EnemyUserProfile | mods\src\prime\BattleResultHeader.h | 50 | manual-review | manual-review |
| member |  |  | scope-ambiguous | EnqueueOrCombineToast | mods\src\patches\parts\disable_banners.cc | 59 | manual-review | manual-review |
| member |  |  | scope-ambiguous | EnqueueToast | mods\src\patches\parts\disable_banners.cc | 53 | manual-review | manual-review |
| member |  |  | scope-ambiguous | EntityGroups | mods\src\prime\ServiceResponse.h | 49 | manual-review | manual-review |
| member |  |  | scope-ambiguous | EnumToKey | mods\src\patches\parts\open_bulk_claim_gifts.cc | 309 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ExtractBuffsOfType | mods\src\patches\parts\misc.cc | 287 | manual-review | manual-review |
| member |  |  | scope-ambiguous | FactionID | mods\src\prime\IBuffComparer.h | 16 | manual-review | manual-review |
| member |  |  | scope-ambiguous | farClipPlane | mods\src\prime\Camera.h | 23 | manual-review | manual-review |
| member |  |  | scope-ambiguous | farClipPlane | mods\src\prime\Camera.h | 28 | manual-review | manual-review |
| member |  |  | scope-ambiguous | fleet | mods\src\prime\FleetLocalViewController.h | 63 | manual-review | manual-review |
| member |  |  | scope-ambiguous | FleetType | mods\src\prime\FleetDeployedData.h | 44 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ForceCompletion | mods\src\prime\BlurController.h | 43 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_childCount | mods\src\patches\parts\loading_screen.cc | 72 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_childCount | mods\src\patches\parts\transition_screen.cc | 60 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_childCount | mods\src\patches\parts\transition_screen.cc | 198 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_FlatRenderable | mods\src\patches\parts\zoom.cc | 330 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_gameObject | mods\src\patches\parts\loading_screen.cc | 74 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_gameObject | mods\src\patches\parts\loading_screen_common.h | 177 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_gameObject | mods\src\patches\parts\transition_screen.cc | 58 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_gameObject | mods\src\patches\parts\transition_screen.cc | 62 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_gameObject | mods\src\patches\parts\transition_screen.cc | 196 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_gameObject | mods\src\patches\parts\transition_screen.cc | 200 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_height | mods\src\patches\parts\loading_screen_common.h | 513 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_name | mods\src\patches\parts\loading_screen.cc | 75 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_name | mods\src\patches\parts\transition_screen.cc | 63 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_name | mods\src\patches\parts\transition_screen.cc | 201 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_parent | mods\src\patches\parts\loading_screen_common.h | 176 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_transform | mods\src\patches\parts\loading_screen.cc | 64 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_transform | mods\src\patches\parts\loading_screen_common.h | 427 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_transform | mods\src\patches\parts\transition_screen.cc | 59 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_transform | mods\src\patches\parts\transition_screen.cc | 120 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_transform | mods\src\patches\parts\transition_screen.cc | 197 | manual-review | manual-review |
| member |  |  | scope-ambiguous | get_width | mods\src\patches\parts\loading_screen_common.h | 512 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetChild | mods\src\patches\parts\loading_screen.cc | 73 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetChild | mods\src\patches\parts\transition_screen.cc | 61 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetChild | mods\src\patches\parts\transition_screen.cc | 199 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetComponent | mods\src\patches\parts\loading_screen.cc | 76 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetComponent | mods\src\patches\parts\loading_screen_common.h | 178 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetComponent | mods\src\patches\parts\loading_screen_common.h | 431 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetComponent | mods\src\patches\parts\transition_screen.cc | 64 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetComponent | mods\src\patches\parts\transition_screen.cc | 202 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetComponentInParent | mods\src\prime\CanvasController.h | 80 | manual-review | manual-review |
| member |  |  | scope-ambiguous | GetMouseWorldPos | mods\src\patches\parts\zoom.cc | 19 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Group | mods\src\prime\EntityGroup.h | 236 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Hull | mods\src\prime\FleetDeployedData.h | 38 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Hull | mods\src\prime\FleetPlayerData.h | 52 | manual-review | manual-review |
| member |  |  | scope-ambiguous | HullID | mods\src\prime\IBuffComparer.h | 10 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ID | mods\src\prime\FleetDeployedData.h | 32 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Id | mods\src\prime\FleetPlayerData.h | 73 | manual-review | manual-review |
| member |  |  | scope-ambiguous | id_ | mods\src\prime\HullSpec.h | 32 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Initialise | mods\src\patches\parts\sync.cc | 2266 | manual-review | manual-review |
| member |  |  | scope-ambiguous | InitializeActions | mods\src\patches\parts\hotkeys.cc | 911 | manual-review | manual-review |
| member |  |  | scope-ambiguous | InitializeAssignmentSorters | mods\src\patches\parts\officer_sort.cc | 491 | manual-review | manual-review |
| member |  |  | scope-ambiguous | InitializeOfficerSorters | mods\src\patches\parts\officer_sort.cc | 485 | manual-review | manual-review |
| member |  |  | scope-ambiguous | InitPrimeServer | mods\src\patches\parts\sync.cc | 2248 | manual-review | manual-review |
| member |  |  | scope-ambiguous | InjectTabData | mods\src\patches\parts\open_bulk_claim_gifts.cc | 323 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Instance | mods\src\prime\MonoSingleton.h | 11 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Instance | mods\src\prime\NavigationSectionManager.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | InstanceIdJson | mods\src\prime\RealtimeDataPayload.h | 47 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Interactable | mods\src\prime\GenericButtonContext.h | 20 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Interactable | mods\src\prime\GenericButtonContext.h | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Interactable | mods\src\prime\SemaphoreButtonListener.h | 28 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Interactable | mods\src\prime\SemaphoreButtonListener.h | 38 | manual-review | manual-review |
| member |  |  | scope-ambiguous | isActiveAndEnabled | mods\src\prime\AssignShipsWidget.h | 30 | manual-review | manual-review |
| member |  |  | scope-ambiguous | isActiveAndEnabled | mods\src\prime\InputFieldWidget.h | 41 | manual-review | manual-review |
| member |  |  | scope-ambiguous | isActiveAndEnabled | mods\src\prime\InventoryListViewController.h | 41 | manual-review | manual-review |
| member |  |  | scope-ambiguous | isActiveAndEnabled | mods\src\prime\OfficerAssignmentViewController.h | 33 | manual-review | manual-review |
| member |  |  | scope-ambiguous | isActiveAndEnabled | mods\src\prime\Widget.h | 39 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsBuffConditionMet | mods\src\patches\parts\buff_fixes.cc | 34 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsDonationUse | mods\src\prime\InventoryForPopup.h | 20 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsDonationUse | mods\src\prime\InventoryForPopup.h | 26 | manual-review | manual-review |
| member |  |  | scope-ambiguous | isFocused | mods\src\prime\TMP_InputField.h | 46 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsInfoShown | mods\src\prime\ObjectViewerBaseWidget.h | 47 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsInfoShown | mods\src\prime\ObjectViewerBaseWidget.h | 53 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsMet | mods\src\prime\ActionRequirement.h | 26 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsQueueUnlocked | mods\src\patches\parts\testing.cc | 201 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsShowing | mods\src\prime\ParentObjectViewerViewController.h | 56 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsSideChatAllowed | mods\src\prime\ChatManager.h | 95 | manual-review | manual-review |
| member |  |  | scope-ambiguous | IsSideChatOpen | mods\src\prime\ChatManager.h | 101 | manual-review | manual-review |
| member |  |  | scope-ambiguous | LateUpdate | mods\src\patches\parts\fix_pan.cc | 67 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Length | mods\src\prime\EntityGroup.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | LoadConfigs | mods\src\patches\parts\testing.cc | 173 | manual-review | manual-review |
| member |  |  | scope-ambiguous | localScale | mods\src\prime\Transform.h | 12 | manual-review | manual-review |
| member |  |  | scope-ambiguous | localScale | mods\src\prime\Transform.h | 18 | manual-review | manual-review |
| member |  |  | scope-ambiguous | m_canvas | mods\src\prime\CanvasController.h | 61 | manual-review | manual-review |
| member |  |  | scope-ambiguous | m_canvasRootScaler | mods\src\prime\ScreenManager.h | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | m_identifier | mods\src\patches\parts\cargo_format.cc | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | m_significantDecimals | mods\src\patches\parts\cargo_format.cc | 30 | manual-review | manual-review |
| member |  |  | scope-ambiguous | m_visible | mods\src\prime\CanvasController.h | 43 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ModifierType | mods\src\prime\IBuffData.h | 22 | manual-review | manual-review |
| member |  |  | scope-ambiguous | name | mods\src\prime\CanvasController.h | 49 | manual-review | manual-review |
| member |  |  | scope-ambiguous | name_ | mods\src\prime\HullSpec.h | 38 | manual-review | manual-review |
| member |  |  | scope-ambiguous | name_ | mods\src\prime\UserProfile.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | NavigationManager | mods\src\prime\NavigationSectionManager.h | 36 | manual-review | manual-review |
| member |  |  | scope-ambiguous | nearClipPlane | mods\src\prime\Camera.h | 34 | manual-review | manual-review |
| member |  |  | scope-ambiguous | nearClipPlane | mods\src\prime\Camera.h | 39 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OfficerSortByBelowDeckAbilityAscending | mods\src\patches\parts\officer_sort.cc | 166 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OfficerSortByBelowDeckAbilityDescending | mods\src\patches\parts\officer_sort.cc | 171 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OfficerSortByIndexAscending | mods\src\patches\parts\officer_sort.cc | 185 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OfficerSortByUnlockedStateAscending | mods\src\patches\parts\officer_sort.cc | 178 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnActionButtonPressedCallback | mods\src\patches\parts\misc.cc | 71 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnDestroy | mods\src\patches\parts\object_tracker.cc | 175 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnDidBindContext | mods\src\patches\parts\hotkeys.cc | 936 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnDidChangeSelectedTab | mods\src\patches\parts\chat.cc | 156 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnGlobalMessageReceived | mods\src\patches\parts\chat.cc | 179 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnOpenButtonClicked | mods\src\patches\parts\open_bulk_claim_gifts.cc | 301 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnPanelFocused | mods\src\patches\parts\chat.cc | 173 | manual-review | manual-review |
| member |  |  | scope-ambiguous | OnRegionalMessageReceived | mods\src\patches\parts\chat.cc | 185 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Parent | mods\src\prime\ObjectViewerBaseWidget.h | 41 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2129 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2146 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2159 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2171 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2183 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2195 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2207 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2219 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObject | mods\src\patches\parts\sync.cc | 2231 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseBinaryObjectsHelper | mods\src\patches\parts\sync.cc | 2103 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseEntitySlotsData | mods\src\patches\parts\sync.cc | 2237 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseFinalPayload | mods\src\patches\parts\sync.cc | 2283 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ParseFinalPayload | mods\src\patches\parts\sync.cc | 2294 | manual-review | manual-review |
| member |  |  | scope-ambiguous | phase | mods\src\prime\TKTouch.h | 27 | manual-review | manual-review |
| member |  |  | scope-ambiguous | phase | mods\src\prime\TKTouch.h | 33 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PlatformApiKey | mods\src\patches\parts\testing.cc | 58 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PlatformApiKey | mods\src\patches\parts\testing.cc | 64 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PlatformSettingsUrl | mods\src\patches\parts\testing.cc | 46 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PlatformSettingsUrl | mods\src\patches\parts\testing.cc | 52 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PlayerShipHullId | mods\src\prime\BattleResultHeader.h | 76 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PlayerUserProfile | mods\src\prime\BattleResultHeader.h | 44 | manual-review | manual-review |
| member |  |  | scope-ambiguous | populateWithPosition | mods\src\patches\parts\fix_pan.cc | 56 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PrepareAllForReload | mods\src\patches\parts\transition_screen.cc | 353 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PreviousState | mods\src\prime\FleetPlayerData.h | 67 | manual-review | manual-review |
| member |  |  | scope-ambiguous | PreviousState | mods\src\prime\StateContainer.h | 29 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ProcessResultInternal | mods\src\patches\parts\sync.cc | 2096 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ProcessResultInternal | mods\src\patches\parts\sync.cc | 2116 | manual-review | manual-review |
| member |  |  | scope-ambiguous | referenceResolution | mods\src\prime\CanvasScaler.h | 44 | manual-review | manual-review |
| member |  |  | scope-ambiguous | referenceResolution | mods\src\prime\CanvasScaler.h | 49 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Request | mods\src\prime\HttpJob.h | 36 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Request | mods\src\prime\HttpResponse.h | 20 | manual-review | manual-review |
| member |  |  | scope-ambiguous | scaleFactor | mods\src\prime\Canvas.h | 19 | manual-review | manual-review |
| member |  |  | scope-ambiguous | scaleFactor | mods\src\prime\Canvas.h | 24 | manual-review | manual-review |
| member |  |  | scope-ambiguous | scaleFactor | mods\src\prime\CanvasScaler.h | 33 | manual-review | manual-review |
| member |  |  | scope-ambiguous | scaleFactor | mods\src\prime\CanvasScaler.h | 38 | manual-review | manual-review |
| member |  |  | scope-ambiguous | scene | mods\src\prime\GameObject.h | 68 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SectionManager | mods\src\prime\Hub.h | 271 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Services | mods\src\prime\Hub.h | 256 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_anchoredPosition | mods\src\patches\parts\loading_screen_common.h | 81 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_anchorMax | mods\src\patches\parts\loading_screen_common.h | 78 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_anchorMin | mods\src\patches\parts\loading_screen_common.h | 77 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_color | mods\src\patches\parts\loading_screen_common.h | 112 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_color | mods\src\patches\parts\loading_screen_common.h | 141 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_enabled | mods\src\patches\parts\transition_screen.cc | 129 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_enabled | mods\src\patches\parts\transition_screen.cc | 276 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_localEulerAngles | mods\src\patches\parts\transition_screen.cc | 169 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_localScale | mods\src\patches\parts\transition_screen.cc | 175 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_MaxItemsToUse | mods\src\patches\parts\misc.cc | 58 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_overrideSprite | mods\src\patches\parts\loading_screen_common.h | 559 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_pivot | mods\src\patches\parts\loading_screen_common.h | 79 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_preserveAspect | mods\src\patches\parts\loading_screen_common.h | 114 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_raycastTarget | mods\src\patches\parts\loading_screen_common.h | 115 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_sizeDelta | mods\src\patches\parts\loading_screen_common.h | 80 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_sprite | mods\src\patches\parts\loading_screen_common.h | 111 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_text | mods\src\patches\parts\loading_tip.cc | 52 | manual-review | manual-review |
| member |  |  | scope-ambiguous | set_type | mods\src\patches\parts\loading_screen_common.h | 113 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetAsFirstSibling | mods\src\patches\parts\loading_screen_common.h | 429 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetCursor_Injected | mods\src\patches\parts\testing.cc | 161 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetDepth | mods\src\patches\parts\zoom.cc | 353 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetInstanceIdHeader | mods\src\patches\parts\sync.cc | 2272 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetLocalTextParameters | mods\src\patches\parts\cargo_format.cc | 75 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetRandomTipLocalisedText | mods\src\patches\parts\loading_tip.cc | 149 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetVerticesDirty | mods\src\patches\parts\loading_screen_common.h | 560 | manual-review | manual-review |
| member |  |  | scope-ambiguous | SetViewParameters | mods\src\patches\parts\zoom.cc | 361 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ShouldShowRevealSequence | mods\src\patches\parts\misc.cc | 299 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ShowCurrentSlide | mods\src\patches\parts\transition_screen.cc | 346 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ShowWithFleet | mods\src\patches\parts\hotkeys.cc | 950 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Source | mods\src\prime\RealtimeDataPayload.h | 59 | manual-review | manual-review |
| member |  |  | scope-ambiguous | State | mods\src\prime\HttpJob.h | 42 | manual-review | manual-review |
| member |  |  | scope-ambiguous | State | mods\src\prime\HttpJob.h | 48 | manual-review | manual-review |
| member |  |  | scope-ambiguous | State | mods\src\prime\Toast.h | 86 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Target | mods\src\prime\RealtimeDataPayload.h | 53 | manual-review | manual-review |
| member |  |  | scope-ambiguous | TargetFleetDeployedData | mods\src\prime\BattleTargetData.h | 21 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ToString | mods\src\patches\notification_service.cc | 465 | manual-review | manual-review |
| member |  |  | scope-ambiguous | transform | mods\src\patches\parts\zoom.cc | 117 | manual-review | manual-review |
| member |  |  | scope-ambiguous | transform | mods\src\prime\CanvasController.h | 55 | manual-review | manual-review |
| member |  |  | scope-ambiguous | TriggerSectionChange | mods\src\patches\parts\open_bulk_claim_gifts.cc | 355 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Type | mods\src\prime\EntityGroup.h | 242 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Type | mods\src\prime\HullSpec.h | 44 | manual-review | manual-review |
| member |  |  | scope-ambiguous | UnityRequest | mods\src\prime\HttpJob.h | 54 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Update | mods\src\patches\parts\free_resize.cc | 169 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Update | mods\src\patches\parts\hotkeys.cc | 923 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Update | mods\src\patches\parts\open_bulk_claim_gifts.cc | 345 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Update | mods\src\patches\parts\zoom.cc | 345 | manual-review | manual-review |
| member |  |  | scope-ambiguous | UpdateCanvasRootScaleFactor | mods\src\patches\parts\ui_scale.cc | 84 | manual-review | manual-review |
| member |  |  | scope-ambiguous | uploadHandler | mods\src\prime\UnityWebRequest.h | 9 | manual-review | manual-review |
| member |  |  | scope-ambiguous | URL | mods\src\prime\HttpRequest.h | 9 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Value | mods\src\patches\parts\misc.cc | 187 | manual-review | manual-review |
| member |  |  | scope-ambiguous | value | mods\src\patches\parts\misc.cc | 208 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ViewMode | mods\src\prime\ChatManager.h | 107 | manual-review | manual-review |
| member |  |  | scope-ambiguous | ViewMode | mods\src\prime\ChatManager.h | 113 | manual-review | manual-review |
| member |  |  | scope-ambiguous | VisibilityState | mods\src\prime\VisibilityController.h | 63 | manual-review | manual-review |
| member |  |  | scope-ambiguous | Visible | mods\src\prime\CanvasController.h | 31 | manual-review | manual-review |
| member |  |  | scope-ambiguous | WndProc | mods\src\patches\parts\free_resize.cc | 175 | manual-review | manual-review |
| member |  |  | scope-ambiguous | x | mods\src\prime\Vector2.h | 22 | manual-review | manual-review |
| member |  |  | scope-ambiguous | x | mods\src\prime\Vector2.h | 28 | manual-review | manual-review |
| member |  |  | scope-ambiguous | x | mods\src\prime\Vector3.h | 25 | manual-review | manual-review |
| member |  |  | scope-ambiguous | x | mods\src\prime\Vector3.h | 30 | manual-review | manual-review |
| member |  |  | scope-ambiguous | y | mods\src\prime\Vector2.h | 34 | manual-review | manual-review |
| member |  |  | scope-ambiguous | y | mods\src\prime\Vector2.h | 40 | manual-review | manual-review |
| member |  |  | scope-ambiguous | y | mods\src\prime\Vector3.h | 35 | manual-review | manual-review |
| member |  |  | scope-ambiguous | y | mods\src\prime\Vector3.h | 40 | manual-review | manual-review |
| member |  |  | scope-ambiguous | z | mods\src\prime\Vector3.h | 45 | manual-review | manual-review |
| member |  |  | scope-ambiguous | z | mods\src\prime\Vector3.h | 50 | manual-review | manual-review |

## Manual review requirements

- Resolve scope-ambiguous member lookups against the owning class helper.
- Resolve dynamic lookup names and overloads from dump/metadata and runtime logs.
- Confirm game-visible signatures and original-call fallbacks before detouring.
- Validate short ARM64 functions and hardcoded offsets with runtime evidence.
