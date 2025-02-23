#pragma once
// internal
#include "QtProjectWizardContent.h"

class SourceGroupSettingsUnloadable;

class QtProjectWizardContentUnloadable : public QtProjectWizardContent {
  Q_OBJECT

public:
  QtProjectWizardContentUnloadable(std::shared_ptr<SourceGroupSettingsUnloadable> settings,
                                   QtProjectWizardWindow* window);

  // QtProjectWizardContent implementation
  void populate(QGridLayout* layout, int& row) override;

private:
  std::shared_ptr<SourceGroupSettingsUnloadable> m_settings;
};
