#include "config.h"
#include "errormsg.h"

#include <il2cpp/il2cpp_helper.h>

#include <EASTL/unordered_set.h>
#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <cstddef>

namespace
{
using EnableForceCommasOverrideFn = void (*)(void*, bool, bool);
using SetBoolPropertyFn           = void (*)(void*, bool);
using SetIntPropertyFn            = void (*)(void*, int);

thread_local int            g_tournament_progress_binding_depth     = 0;
EnableForceCommasOverrideFn g_enable_force_commas_override          = nullptr;
SetBoolPropertyFn           g_set_context_use_culture_variant       = nullptr;
SetBoolPropertyFn           g_set_context_override_number_settings  = nullptr;
SetBoolPropertyFn           g_set_context_force_localize_params     = nullptr;
SetBoolPropertyFn           g_set_context_truncate_decimals         = nullptr;
SetBoolPropertyFn           g_set_context_abbreviate_numbers        = nullptr;
SetIntPropertyFn            g_set_context_significant_decimals      = nullptr;
SetBoolPropertyFn           g_set_localizer_force_localize_params   = nullptr;
Il2CppClass*                g_locale_context_cls                    = nullptr;
Il2CppClass*                g_simple_progress_bar_cls               = nullptr;
Il2CppClass*                g_legacy_progress_bar_cls               = nullptr;
ptrdiff_t                   g_localizer_force_commas_offset         = 0;
ptrdiff_t                   g_localizer_use_culture_variant_offset  = 0;
ptrdiff_t                   g_localizer_truncate_decimals_offset    = 0;
ptrdiff_t                   g_localizer_significant_decimals_offset = 0;
ptrdiff_t                   g_localizer_abbreviate_numbers_offset   = 0;
ptrdiff_t                   g_score_next_reward_text_offset         = 0;
ptrdiff_t                   g_progress_bar_offset                   = 0;
ptrdiff_t                   g_face_off_progress_bar_offset          = 0;
ptrdiff_t                   g_widget_score_offset                   = 0;
ptrdiff_t                   g_widget_heroic_score_offset            = 0;
ptrdiff_t                   g_widget_progress_bar_offset            = 0;
ptrdiff_t                   g_dynamic_value_localizer_offset        = 0;
ptrdiff_t                   g_task_details_bar_offset               = 0;
ptrdiff_t                   g_legacy_value_offsets[7]               = {};
bool                        g_cfg_use_locale                        = true;
int                         g_cfg_decimals                          = -1;
bool                        g_cfg_abbreviate                        = false;

// Localizers that render tournament score/progress values. Once a localizer is
// registered, its formatting flags are (re)applied on every parameter bind, so
// reactive updates stay formatted even when they run outside an initialise call.
eastl::unordered_set<void*> g_format_localizers;

class TournamentProgressBindingScope
{
public:
  TournamentProgressBindingScope()
  {
    ++g_tournament_progress_binding_depth;
  }

  ~TournamentProgressBindingScope()
  {
    --g_tournament_progress_binding_depth;
  }
};

void* ReadFieldReference(void* owner, ptrdiff_t field_offset)
{
  if (!owner || !field_offset) {
    return nullptr;
  }

  return *reinterpret_cast<void**>(reinterpret_cast<char*>(owner) + field_offset);
}

void EnableLocalizerNumberFormatting(void* text_localizer)
{
  if (!text_localizer) {
    return;
  }

  g_set_localizer_force_localize_params(text_localizer, true);
  *reinterpret_cast<bool*>(reinterpret_cast<char*>(text_localizer) + g_localizer_force_commas_offset) = true;
  *reinterpret_cast<bool*>(reinterpret_cast<char*>(text_localizer) + g_localizer_use_culture_variant_offset) =
      g_cfg_use_locale;
  *reinterpret_cast<bool*>(reinterpret_cast<char*>(text_localizer) + g_localizer_abbreviate_numbers_offset) =
      g_cfg_abbreviate;
  if (g_cfg_decimals >= 0) {
    *reinterpret_cast<bool*>(reinterpret_cast<char*>(text_localizer) + g_localizer_truncate_decimals_offset) = true;
    *reinterpret_cast<int*>(reinterpret_cast<char*>(text_localizer) + g_localizer_significant_decimals_offset) =
        g_cfg_decimals;
  }
}

void ApplyContextNumberFormatting(void* context)
{
  if (!context) {
    return;
  }

  // SetContext only copies the context number settings onto the localizer when
  // OverrideNumberSettings is set; otherwise it restores the localizer defaults.
  g_set_context_override_number_settings(context, true);
  g_set_context_use_culture_variant(context, g_cfg_use_locale);
  g_set_context_force_localize_params(context, true);
  g_enable_force_commas_override(context, true, true);
  g_set_context_abbreviate_numbers(context, g_cfg_abbreviate);
  if (g_cfg_decimals >= 0) {
    g_set_context_truncate_decimals(context, true);
    g_set_context_significant_decimals(context, g_cfg_decimals);
  }
}

void TrackLocalizer(void* text_localizer)
{
  if (!text_localizer) {
    return;
  }

  EnableLocalizerNumberFormatting(text_localizer);
  g_format_localizers.insert(text_localizer);
}

void TrackProgressBarDynamicValue(void* owner, ptrdiff_t progress_bar_offset)
{
  auto* progress_bar = ReadFieldReference(owner, progress_bar_offset);
  TrackLocalizer(ReadFieldReference(progress_bar, g_dynamic_value_localizer_offset));
}

void TrackSectionLocalizers(void* controller)
{
  TrackProgressBarDynamicValue(controller, g_progress_bar_offset);
  TrackProgressBarDynamicValue(controller, g_face_off_progress_bar_offset);
  TrackLocalizer(ReadFieldReference(controller, g_score_next_reward_text_offset));
}

// Alliance Games task details bind a ProgressData to _progressBarWidget. The
// prefab holds a SimpleProgressBarWidget; keep a LegacyProgressBar fallback in
// case other prefabs differ.
void TrackTaskDetailsProgressBar(void* task_widget)
{
  auto* bar   = ReadFieldReference(task_widget, g_task_details_bar_offset);
  auto* klass = bar ? reinterpret_cast<Il2CppObject*>(bar)->klass : nullptr;
  if (!klass) {
    return;
  }

  if (il2cpp_class_is_assignable_from(g_simple_progress_bar_cls, klass)) {
    TrackLocalizer(ReadFieldReference(bar, g_dynamic_value_localizer_offset));
    return;
  }
  if (il2cpp_class_is_assignable_from(g_legacy_progress_bar_cls, klass)) {
    for (auto offset : g_legacy_value_offsets) {
      TrackLocalizer(ReadFieldReference(bar, offset));
    }
  }
}

bool IsLocaleTextContext(void* obj)
{
  if (!obj) {
    return false;
  }

  auto* klass = reinterpret_cast<Il2CppObject*>(obj)->klass;
  return klass && il2cpp_class_is_assignable_from(g_locale_context_cls, klass);
}

void FormatContextArgs(Il2CppArray* args)
{
  if (!args) {
    return;
  }

  auto len = reinterpret_cast<Il2CppArraySize*>(args)->max_length;
  for (il2cpp_array_size_t i = 0; i < len; ++i) {
    auto* arg = reinterpret_cast<Il2CppObject**>(reinterpret_cast<Il2CppArraySize*>(args)->vector)[i];
    if (IsLocaleTextContext(arg)) {
      ApplyContextNumberFormatting(arg);
    }
  }
}

bool ShouldFormat(void* text_localizer)
{
  return g_tournament_progress_binding_depth > 0 || g_format_localizers.contains(text_localizer);
}
} // namespace

void TournamentDetailSectionViewController_InitialiseFromContext_Hook(auto original, void* controller)
{
  TournamentProgressBindingScope scope;
  TrackSectionLocalizers(controller);
  original(controller);
}

void TournamentDetailSectionViewController_InitialiseDetailPanel_Hook(auto original, void* controller, void* tournament)
{
  TournamentProgressBindingScope scope;
  TrackSectionLocalizers(controller);
  original(controller, tournament);
}

void TournamentDetailSectionViewController_InitialiseProgressBar_Hook(auto original, void* controller)
{
  TournamentProgressBindingScope scope;
  TrackSectionLocalizers(controller);
  original(controller);
}

void TournamentDetailSectionViewController_InitialiseNextMilestoneReward_Hook(auto original, void* controller)
{
  TournamentProgressBindingScope scope;
  TrackSectionLocalizers(controller);
  original(controller);
}

void TournamentDetailProgressWidget_InitialiseProgressBar_Hook(auto original, void* widget)
{
  TournamentProgressBindingScope scope;
  TrackProgressBarDynamicValue(widget, g_widget_progress_bar_offset);
  original(widget);
}

void TournamentDetailProgressWidget_SetWidgetData_Hook(auto original, void* widget)
{
  TournamentProgressBindingScope scope;
  TrackLocalizer(ReadFieldReference(widget, g_widget_score_offset));
  TrackLocalizer(ReadFieldReference(widget, g_widget_heroic_score_offset));
  TrackProgressBarDynamicValue(widget, g_widget_progress_bar_offset);
  original(widget);
}

void TaskDetailsWidget_SetWidgetData_Hook(auto original, void* widget)
{
  TournamentProgressBindingScope scope;
  TrackTaskDetailsProgressBar(widget);
  original(widget);
}

void TaskDetailsWidget_SetProgressBarData_Hook(auto original, void* widget)
{
  TournamentProgressBindingScope scope;
  TrackTaskDetailsProgressBar(widget);
  original(widget);
}

void TournamentProgress_TextLocalizer_SetContext_Hook(auto original, void* text_localizer, void* context)
{
  if (ShouldFormat(text_localizer)) {
    ApplyContextNumberFormatting(context);
    TrackLocalizer(text_localizer);
  }

  original(text_localizer, context);
}

void TournamentProgress_TextLocalizer_SetLocalTextParameters_Hook(auto    original,
                                                                  void*   text_localizer,
                                                                  bool    parse_id,
                                                                  Il2CppArray* args)
{
  if (ShouldFormat(text_localizer)) {
    EnableLocalizerNumberFormatting(text_localizer);
    FormatContextArgs(args);
  }

  original(text_localizer, parse_id, args);
}

void InstallTournamentProgressFormatHooks()
{
  const auto& config  = Config::Get();
  g_cfg_use_locale    = config.tournament_progress_use_locale;
  g_cfg_decimals      = config.tournament_progress_decimals;
  g_cfg_abbreviate    = config.tournament_progress_abbreviate;

  auto locale_context_helper =
      il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "LocaleTextContext");
  if (!locale_context_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Client.UI", "LocaleTextContext");
    return;
  }

  g_locale_context_cls = locale_context_helper.get_cls();
  g_enable_force_commas_override = reinterpret_cast<EnableForceCommasOverrideFn>(
      locale_context_helper.GetMethod("EnableForceCommasOverride", 2));
  g_set_context_use_culture_variant =
      reinterpret_cast<SetBoolPropertyFn>(locale_context_helper.GetMethod("set_UseCultureVariant", 1));
  g_set_context_override_number_settings =
      reinterpret_cast<SetBoolPropertyFn>(locale_context_helper.GetMethod("set_OverrideNumberSettings", 1));
  g_set_context_force_localize_params =
      reinterpret_cast<SetBoolPropertyFn>(locale_context_helper.GetMethod("set_ForceLocalizeParams", 1));
  g_set_context_truncate_decimals =
      reinterpret_cast<SetBoolPropertyFn>(locale_context_helper.GetMethod("set_TruncateDecimals", 1));
  g_set_context_abbreviate_numbers =
      reinterpret_cast<SetBoolPropertyFn>(locale_context_helper.GetMethod("set_AbbreviateNumbers", 1));
  g_set_context_significant_decimals =
      reinterpret_cast<SetIntPropertyFn>(locale_context_helper.GetMethod("set_SignificantDecimals", 1));
  if (!g_enable_force_commas_override || !g_set_context_use_culture_variant
      || !g_set_context_override_number_settings || !g_set_context_force_localize_params
      || !g_set_context_truncate_decimals || !g_set_context_abbreviate_numbers
      || !g_set_context_significant_decimals) {
    ErrorMsg::MissingMethod("LocaleTextContext", "number formatting methods");
    return;
  }

  auto text_localizer_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "TextLocalizer");
  if (!text_localizer_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Client.UI", "TextLocalizer");
    return;
  }

  auto set_context               = text_localizer_helper.GetMethod("SetContext", 1);
  auto set_local_text_parameters = text_localizer_helper.GetMethod("SetLocalTextParameters", 2);
  g_set_localizer_force_localize_params =
      reinterpret_cast<SetBoolPropertyFn>(text_localizer_helper.GetMethod("set_ForceLocalizeParams", 1));
  if (!set_context || !set_local_text_parameters || !g_set_localizer_force_localize_params) {
    ErrorMsg::MissingMethod("TextLocalizer", "number localization methods");
    return;
  }

  auto force_commas_field        = text_localizer_helper.GetField("m_forceCommas");
  auto use_culture_variant_field = text_localizer_helper.GetField("m_useCultureVariant");
  auto truncate_decimals_field   = text_localizer_helper.GetField("m_truncateDecimals");
  auto significant_field         = text_localizer_helper.GetField("m_significantDecimals");
  auto abbreviate_field          = text_localizer_helper.GetField("m_abbreviateNumbers");
  if (!force_commas_field.isValidHelper() || !use_culture_variant_field.isValidHelper()
      || !truncate_decimals_field.isValidHelper() || !significant_field.isValidHelper()
      || !abbreviate_field.isValidHelper()) {
    ErrorMsg::MissingMethod("TextLocalizer", "number formatting fields");
    return;
  }
  g_localizer_force_commas_offset         = force_commas_field.offset();
  g_localizer_use_culture_variant_offset  = use_culture_variant_field.offset();
  g_localizer_truncate_decimals_offset    = truncate_decimals_field.offset();
  g_localizer_significant_decimals_offset = significant_field.offset();
  g_localizer_abbreviate_numbers_offset   = abbreviate_field.offset();

  auto progress_bar_helper =
      il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.UI", "SimpleProgressBarWidget");
  if (!progress_bar_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.UI", "SimpleProgressBarWidget");
    return;
  }

  auto dynamic_value_field = progress_bar_helper.GetField("_currentDynamicValue");
  if (!dynamic_value_field.isValidHelper()) {
    ErrorMsg::MissingMethod("SimpleProgressBarWidget", "_currentDynamicValue");
    return;
  }
  g_dynamic_value_localizer_offset = dynamic_value_field.offset();
  g_simple_progress_bar_cls        = progress_bar_helper.get_cls();

  auto detail_controller_helper =
      il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Tournaments", "TournamentDetailSectionViewController");
  if (!detail_controller_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Tournaments", "TournamentDetailSectionViewController");
    return;
  }

  auto detail_progress_widget_helper =
      il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Tournaments", "TournamentDetailProgressWidget");
  if (!detail_progress_widget_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Tournaments", "TournamentDetailProgressWidget");
    return;
  }

  auto initialise_detail_progress = detail_controller_helper.GetMethod("InitialiseProgressBar", 0);
  auto initialise_next_reward     = detail_controller_helper.GetMethod("InitialiseNextMilestoneReward", 0);
  auto initialise_detail_panel    = detail_controller_helper.GetMethod("InitialiseDetailPanel", 1);
  auto initialise_from_context    = detail_controller_helper.GetMethod("InitialiseFromContext", 0);
  auto initialise_tier_progress   = detail_progress_widget_helper.GetMethod("InitialiseProgressBar", 0);
  auto set_tier_widget_data       = detail_progress_widget_helper.GetMethod("SetWidgetData", 0);
  if (!initialise_detail_progress || !initialise_next_reward || !initialise_detail_panel
      || !initialise_from_context) {
    ErrorMsg::MissingMethod("TournamentDetailSectionViewController", "progress initialisers");
    return;
  }
  if (!initialise_tier_progress || !set_tier_widget_data) {
    ErrorMsg::MissingMethod("TournamentDetailProgressWidget", "progress initialisers");
    return;
  }

  auto score_next_reward_field = detail_controller_helper.GetField("_scoreNextRewardText");
  auto progress_bar_field      = detail_controller_helper.GetField("_progressBar");
  auto face_off_bar_field      = detail_controller_helper.GetField("_faceOffProgressBar");
  auto widget_score_field      = detail_progress_widget_helper.GetField("_score");
  auto widget_heroic_field     = detail_progress_widget_helper.GetField("_heroicScore");
  auto widget_bar_field        = detail_progress_widget_helper.GetField("_progressBar");
  if (!score_next_reward_field.isValidHelper() || !progress_bar_field.isValidHelper()
      || !face_off_bar_field.isValidHelper() || !widget_score_field.isValidHelper()
      || !widget_heroic_field.isValidHelper() || !widget_bar_field.isValidHelper()) {
    ErrorMsg::MissingMethod("TournamentDetail widgets", "score/progress fields");
    return;
  }
  g_score_next_reward_text_offset = score_next_reward_field.offset();
  g_progress_bar_offset           = progress_bar_field.offset();
  g_face_off_progress_bar_offset  = face_off_bar_field.offset();
  g_widget_score_offset           = widget_score_field.offset();
  g_widget_heroic_score_offset    = widget_heroic_field.offset();
  g_widget_progress_bar_offset    = widget_bar_field.offset();

  // Alliance Games task details render their progress through _progressBarWidget.
  auto task_details_helper =
      il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Alliances.Games", "TaskDetailsWidget");
  auto legacy_bar_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "LegacyProgressBar");
  if (!task_details_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Prime.Alliances.Games", "TaskDetailsWidget");
    return;
  }
  if (!legacy_bar_helper.isValidHelper()) {
    ErrorMsg::MissingHelper("Digit.Client.UI", "LegacyProgressBar");
    return;
  }

  g_legacy_progress_bar_cls = legacy_bar_helper.get_cls();

  auto task_bar_field       = task_details_helper.GetField("_progressBarWidget");
  auto task_set_widget_data = task_details_helper.GetMethod("SetWidgetData", 0);
  auto task_set_bar_data    = task_details_helper.GetMethod("SetProgressBarData", 0);
  if (!task_bar_field.isValidHelper() || !task_set_widget_data || !task_set_bar_data) {
    ErrorMsg::MissingMethod("TaskDetailsWidget", "progress bar members");
    return;
  }
  g_task_details_bar_offset = task_bar_field.offset();

  static const char* const legacy_value_fields[7] = {
      "_currentDynamicValue", "_currentValue", "_currentValue_X_of_Y", "_compoundValue",
      "_percentValue",        "_minValue",     "_maxValue"};
  for (int i = 0; i < 7; ++i) {
    auto field = legacy_bar_helper.GetField(legacy_value_fields[i]);
    if (!field.isValidHelper()) {
      ErrorMsg::MissingMethod("LegacyProgressBar", legacy_value_fields[i]);
      return;
    }
    g_legacy_value_offsets[i] = field.offset();
  }

  SPUD_STATIC_DETOUR(set_context, TournamentProgress_TextLocalizer_SetContext_Hook);
  SPUD_STATIC_DETOUR(set_local_text_parameters, TournamentProgress_TextLocalizer_SetLocalTextParameters_Hook);
  SPUD_STATIC_DETOUR(initialise_detail_progress,
                     TournamentDetailSectionViewController_InitialiseProgressBar_Hook);
  SPUD_STATIC_DETOUR(initialise_next_reward,
                     TournamentDetailSectionViewController_InitialiseNextMilestoneReward_Hook);
  SPUD_STATIC_DETOUR(initialise_detail_panel,
                     TournamentDetailSectionViewController_InitialiseDetailPanel_Hook);
  SPUD_STATIC_DETOUR(initialise_from_context,
                     TournamentDetailSectionViewController_InitialiseFromContext_Hook);
  SPUD_STATIC_DETOUR(initialise_tier_progress, TournamentDetailProgressWidget_InitialiseProgressBar_Hook);
  SPUD_STATIC_DETOUR(set_tier_widget_data, TournamentDetailProgressWidget_SetWidgetData_Hook);
  SPUD_STATIC_DETOUR(task_set_widget_data, TaskDetailsWidget_SetWidgetData_Hook);
  SPUD_STATIC_DETOUR(task_set_bar_data, TaskDetailsWidget_SetProgressBarData_Hook);
  spdlog::info("Tournament progress format: separators on (locale={} decimals={} abbreviate={})",
               g_cfg_use_locale, g_cfg_decimals, g_cfg_abbreviate);
}
