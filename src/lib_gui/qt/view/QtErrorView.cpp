#include "qt/view/QtErrorView.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QStyledItemDelegate>

#include "qt/element/QtHelpButton.h"
#include "qt/element/QtTable.h"
#include "qt/utility/utilityQt.h"
#include "qt/view/QtViewWidgetWrapper.h"
#include "settings/ColorScheme.h"
#include "utility/messaging/type/MessageErrorFilterChanged.h"
#include "utility/messaging/type/MessageProjectEdit.h"
#include "utility/messaging/type/MessageShowErrors.h"
#include "utility/ResourcePaths.h"

QIcon QtErrorView::s_errorIcon;

class SelectableDelegate
	: public QStyledItemDelegate
{
public:
	SelectableDelegate(QObject* parent = Q_NULLPTR);
	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

SelectableDelegate::SelectableDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}


QWidget* SelectableDelegate::createEditor(
	QWidget* parent,
	const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
	QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(editor);
	if (lineEdit != nullptr)
	{
		lineEdit->setReadOnly(true);
	}
	return editor;
}

QtErrorView::QtErrorView(ViewLayout* viewLayout)
	: ErrorView(viewLayout)
	, m_ignoreRowSelection(false)
{
	s_errorIcon = QIcon(QString::fromStdWString(ResourcePaths::getGuiPath().concatenate(L"indexing_dialog/error.png").wstr()));
}

QtErrorView::~QtErrorView()
{
}

void QtErrorView::createWidgetWrapper()
{
	setWidgetWrapper(std::make_shared<QtViewWidgetWrapper>(new QFrame()));
}

void QtErrorView::initView()
{
	QWidget* widget = QtViewWidgetWrapper::getWidgetOfView(this);

	QBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	widget->setLayout(layout);

	m_table = new QtTable(this);
	m_model = new QStandardItemModel(this);
	m_table->setModel(m_model);
	m_table->setItemDelegate(new SelectableDelegate(m_table));

	// Setup Table Headers
	m_model->setColumnCount(COLUMN_MAX + 1);
	m_table->setColumnWidth(COLUMN::TYPE, 80);
	m_table->setColumnWidth(COLUMN::MESSAGE, 450);
	m_table->setColumnWidth(COLUMN::FILE, 300);
	m_table->setColumnWidth(COLUMN::LINE, 50);
	m_table->setColumnHidden(COLUMN::ID, true);

	QStringList headers;
	headers << "Type" << "Message" << "File" << "Line" << "Indexed";
	m_model->setHorizontalHeaderLabels(headers);

	connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged,
		[=](const QModelIndex& index, const QModelIndex& previousIndex)
	{
		if (index.isValid() && !m_ignoreRowSelection)
		{
			if (m_model->item(index.row(), COLUMN::FILE) == nullptr)
			{
				return;
			}

			MessageShowErrors(m_model->item(index.row(), COLUMN::ID)->text().toUInt()).dispatch();
		}
	});

	layout->addWidget(m_table);

	// Setup Checkboxes
	QBoxLayout* checkboxes = new QHBoxLayout();
	checkboxes->setContentsMargins(10, 3, 0, 3);
	checkboxes->setSpacing(0);

	{
		m_showFatals = createFilterCheckbox("Fatals", m_errorFilter.fatal, checkboxes);
		m_showErrors = createFilterCheckbox("Errors", m_errorFilter.error, checkboxes);
		m_showNonIndexedFatals = createFilterCheckbox("Fatals in non-indexed files", m_errorFilter.unindexedFatal, checkboxes);
		m_showNonIndexedErrors = createFilterCheckbox("Errors in non-indexed files", m_errorFilter.unindexedError, checkboxes);

		m_helpButton = new QtHelpButton("Fixing Errors",
			"Please read this if your project is showing errors after indexing.<br />"
			"There are different types of errors:"
			"<ul>"
			"<li><b>Fatals</b> cause the indexer to stop. All or big parts of indexed information for the involved "
			"source file is missing. <b>Make sure to fix all fatals.</li>"
			"<li><b>Errors</b> are issues that the indexer considers as wrong code. Usually the indexer is able to "
			"recover from these errors. The indexed information of the involved source file may not be complete, but "
			"it is still useful.</li>"
			"</ul>"
			"You need to edit your Sourcetrail project and reindex it to fix errors. The displayed error messages are "
			"generated by Sourcetrail's language specific indexers:"
			"<ul>"
			"<li><b>C/C++</b> error messages are generated by the <b>clang compiler frontend</b>.</li>"
			"<li><b>Java</b> error messages are generated by the <b>Eclipse JDT library</b>.</li>"
			"</ul>"
			"You should be able to find information about errors you are not familiar with online. <b>Double click</b> "
			"an error message in the table to select it for <b>copying</b>.<br />"
		);
		m_helpButton->setObjectName("help_button");
		checkboxes->addWidget(m_helpButton);
	}

	checkboxes->addStretch();

	{
		m_allLabel = new QLabel("");
		checkboxes->addWidget(m_allLabel);
		m_allLabel->hide();
	}

	checkboxes->addSpacing(5);

	{
		m_allButton = new QPushButton("");
		connect(m_allButton, &QPushButton::clicked,
			[=]()
			{
				m_errorFilter.limit = 0;
				errorFilterChanged();
			}
		);
		checkboxes->addWidget(m_allButton);
		m_allButton->hide();
	}

	checkboxes->addSpacing(10);


	{
		QPushButton* editButton = new QPushButton("Edit Project");
		connect(editButton, &QPushButton::clicked,
			[]()
			{
				MessageProjectEdit().dispatch();
			}
		);
		checkboxes->addWidget(editButton);

		editButton->setToolTip("edit project");
		editButton->setIcon(QPixmap(QString::fromStdWString(ResourcePaths::getGuiPath().concatenate(L"code_view/images/edit.png").wstr())));
	}

	checkboxes->addSpacing(10);

	layout->addLayout(checkboxes);
}

void QtErrorView::refreshView()
{
	m_onQtThread([=]()
	{
		setStyleSheet();
	});
}

void QtErrorView::clear()
{
	m_onQtThread([=]()
	{
		if (!m_model->index(0, 0).data(Qt::DisplayRole).toString().isEmpty())
		{
			m_model->removeRows(0, m_model->rowCount());
		}

		m_table->updateRows();
	});
}

void QtErrorView::addErrors(const std::vector<ErrorInfo>& errors, bool scrollTo)
{
	m_onQtThread([=]()
	{
		for (const ErrorInfo& error : errors)
		{
			addErrorToTable(error);
		}
		m_table->updateRows();

		if (scrollTo)
		{
			m_table->showLastRow();
		}
		else
		{
			m_table->showFirstRow();
		}
	});
}

void QtErrorView::setErrorId(Id errorId)
{
	m_onQtThread([=]()
	{
		QList<QStandardItem*> items = m_model->findItems(QString::number(errorId), Qt::MatchExactly, COLUMN::ID);

		if (items.size() == 1)
		{
			m_ignoreRowSelection = true;
			m_table->selectRow(items.at(0)->row());
			m_ignoreRowSelection = false;
		}
	});
}

void QtErrorView::setErrorCount(ErrorCountInfo info)
{
	m_onQtThread([=]()
	{
		m_allLabel->setVisible(m_errorFilter.limit > 0 && info.total > m_errorFilter.limit);
		m_allButton->setVisible(m_errorFilter.limit > 0 && info.total > m_errorFilter.limit);

		m_allLabel->setText("<b>Only showing first " + QString::number(m_errorFilter.limit) + " errors</b>");
		m_allButton->setText("Show all " + QString::number(info.total));
	});
}

void QtErrorView::resetErrorLimit()
{
	ErrorFilter filter;

	m_errorFilter.limit = filter.limit;
	errorFilterChanged(0, false);
}

void QtErrorView::showErrorHelpMessage()
{
	m_onQtThread([=]()
	{
		m_helpButton->click();
	});
}

void QtErrorView::errorFilterChanged(int i)
{
	errorFilterChanged(i, true);
}

void QtErrorView::errorFilterChanged(int i, bool showErrors)
{
	m_table->selectionModel()->clearSelection();

	m_errorFilter.error = m_showErrors->isChecked();
	m_errorFilter.fatal = m_showFatals->isChecked();
	m_errorFilter.unindexedError = m_showNonIndexedErrors->isChecked();
	m_errorFilter.unindexedFatal = m_showNonIndexedFatals->isChecked();

	MessageErrorFilterChanged(m_errorFilter, showErrors).dispatch();
}

void QtErrorView::setStyleSheet() const
{
	QWidget* widget = QtViewWidgetWrapper::getWidgetOfView(this);
	utility::setWidgetBackgroundColor(widget, ColorScheme::getInstance()->getColor("error/background"));

	QPalette palette(m_showErrors->palette());
	palette.setColor(QPalette::WindowText, QColor(ColorScheme::getInstance()->getColor("table/text/normal").c_str()));

	m_showErrors->setPalette(palette);
	m_showFatals->setPalette(palette);
	m_showNonIndexedErrors->setPalette(palette);
	m_showNonIndexedFatals->setPalette(palette);

	m_helpButton->setColor(QColor(ColorScheme::getInstance()->getColor("table/text/normal").c_str()));

	m_table->updateRows();
}

void QtErrorView::addErrorToTable(const ErrorInfo& error)
{
	if (!isShownError(error))
	{
		return;
	}

	int rowNumber = m_table->getFilledRowCount();
	if (rowNumber < m_model->rowCount())
	{
		m_model->insertRow(rowNumber);
	}

	m_model->setItem(rowNumber, COLUMN::TYPE, new QStandardItem(error.fatal ? "FATAL" : "ERROR"));
	if (error.fatal)
	{
		m_model->item(rowNumber, COLUMN::TYPE)->setForeground(QBrush(Qt::red));
	}
	m_model->item(rowNumber, COLUMN::TYPE)->setIcon(s_errorIcon);

	m_model->setItem(rowNumber, COLUMN::MESSAGE, new QStandardItem(QString::fromStdWString(error.message)));

	m_model->setItem(rowNumber, COLUMN::FILE, new QStandardItem(QString::fromStdWString(error.filePath)));
	m_model->item(rowNumber, COLUMN::FILE)->setToolTip(QString::fromStdWString(error.filePath));

	m_model->setItem(rowNumber, COLUMN::LINE, new QStandardItem(QString::number(error.lineNumber)));

	m_model->setItem(rowNumber, COLUMN::INDEXED, new QStandardItem(error.indexed ? "yes" : "no"));

	m_model->setItem(rowNumber, COLUMN::ID, new QStandardItem(QString::number(error.id)));
}

QCheckBox* QtErrorView::createFilterCheckbox(const QString& name, bool checked, QBoxLayout* layout)
{
	QCheckBox* checkbox = new QCheckBox(name);
	checkbox->setChecked(checked);

	connect(checkbox, &QCheckBox::stateChanged, this, static_cast<void (QtErrorView::*)(int)>(&QtErrorView::errorFilterChanged));

	layout->addWidget(checkbox);
	layout->addSpacing(25);

	return checkbox;
}

bool QtErrorView::isShownError(const ErrorInfo& error)
{
	if (!error.fatal && error.indexed && m_showErrors->isChecked())
	{
		return true;
	}
	if (error.fatal && error.indexed && m_showFatals->isChecked())
	{
		return true;
	}
	if (!error.fatal && !error.indexed && m_showNonIndexedErrors->isChecked())
	{
		return true;
	}
	if (error.fatal && !error.indexed && m_showNonIndexedFatals->isChecked())
	{
		return true;
	}
	return false;
}

