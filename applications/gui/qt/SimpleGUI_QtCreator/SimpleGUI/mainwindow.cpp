#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <fstream>
#include <streambuf>
#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow) {
	ui->setupUi(this);
	ui->dockWidgetContentsConsole->setMaximumHeight(150);
    ui->dockWidgetContentsPlugin->setMaximumWidth(200);
    ui->listWidget_Plugins->setSortingEnabled(true);
    //ui->treeWidget_Plugins->setVisible(false);
	simulator = new Simulator();
	simulator->getTracer()->setTraceLevel(Util::TraceLevel::L9_mostDetailed);
	simulator->getTracer()->addTraceHandler<MainWindow>(this, &MainWindow::_simulatorTraceHandler);
	simulator->getTracer()->addTraceErrorHandler<MainWindow>(this, &MainWindow::_simulatorTraceErrorHandler);
	simulator->getTracer()->addTraceReportHandler<MainWindow>(this, &MainWindow::_simulatorTraceReportsHandler);
	simulator->getTracer()->addTraceSimulationHandler<MainWindow>(this, &MainWindow::_simulatorTraceSimulationHandler);
	_insertFakePlugins();
    //...
}

MainWindow::~MainWindow() {
	delete ui;
}


//-----------------
// View
//-----------------

void MainWindow::_actualizeWidgets() {
	bool opened = simulator->getModels()->current() != nullptr;
	bool running = false;
	bool paused = false;
	if (opened) {
		running = simulator->getModels()->current()->getSimulation()->isRunning();
		paused = simulator->getModels()->current()->getSimulation()->isPaused();
	}
	ui->actionSave->setEnabled(opened);
	ui->actionClose->setEnabled(opened);
	ui->actionCheck->setEnabled(opened);
	ui->menuSimulation->setEnabled(opened);
    ui->actionStart->setEnabled(opened && !running);
    ui->actionStep->setEnabled(opened && !running);
	ui->actionStop->setEnabled(opened && (running || paused));
	ui->actionPause->setEnabled(opened && running);
	ui->actionResume->setEnabled(opened && paused);
	ui->tabWidgetModel->setEnabled(opened);
    ui->tabSimulation->setEnabled(opened);
    ui->tabReport->setEnabled(opened);
    if (!opened) {
		_clearModelEditors();
	}
}

void MainWindow::_actualizeModelTextHasChanged(bool hasChanged) {
	if (_textModelHasChanged != hasChanged) {
		QString text = "Model";
		if (hasChanged) {
			text += "*";
		}
		ui->tabWidgetModel->setTabText(0, text);
	}
	_textModelHasChanged = hasChanged;

}

void MainWindow::_insertCommandInConsole(std::string text) {
    ui->textEdit_Console->setTextColor(QColor::fromRgb(0, 128, 0));
    QFont font(ui->textEdit_Console->font());
    font.setBold(true);
    ui->textEdit_Console->setFont(font);
	ui->textEdit_Console->append("\n$genesys> " + QString::fromStdString(text));
    ui->textEdit_Console->moveCursor(QTextCursor::MoveOperation::Down, QTextCursor::MoveMode::MoveAnchor);
    font.setBold(false);
    ui->textEdit_Console->setFont(font);
}

void MainWindow::_clearModelEditors() {
	ui->textEdit_Model->clear();
	ui->textEdit_Simulation->clear();
	ui->textEdit_Reports->clear();
}

bool MainWindow::_checkStartSimulation() {
    Model* model = simulator->getModels()->current();
    if (this->_textModelHasChanged) {
        simulator->getModels()->remove(model);
        model = nullptr;
    }
    if (model == nullptr) { // only model text written in UI
        QString modelLanguage = ui->textEdit_Model->toPlainText();
        if (!simulator->getModels()->createFromLanguage(modelLanguage.toStdString())) {
            QMessageBox::critical(this, "Chek Model Simulation", "Error in the model text. See console for more information.");
        }
        model = simulator->getModels()->current();
        if (model != nullptr) {
            _setOnEventHandlers();
        }
    }
    if (model != nullptr) {
        ModelSimulation* sim = model->getSimulation();
        if (!sim->isPaused() && !sim->isRunning()) {
            ui->textEdit_Simulation->clear();
            ui->textEdit_Reports->clear();
        }
        ui->tabWidgetModel->setCurrentIndex(1);
    }
    return simulator->getModels()->current() != nullptr;
}

//-------------------------
// Simulator Trace Handlers
//-------------------------

void MainWindow::_simulatorTraceHandler(TraceEvent e) {
	if (e.getTracelevel() == Util::TraceLevel::L1_errorFatal)
		ui->textEdit_Console->setTextColor(QColor::fromRgb(255, 0, 0));
	else if (e.getTracelevel() == Util::TraceLevel::L2_results)
		ui->textEdit_Console->setTextColor(QColor::fromRgb(0, 0, 255));
	else if (e.getTracelevel() == Util::TraceLevel::L3_errorRecover)
		ui->textEdit_Console->setTextColor(QColor::fromRgb(223, 0, 0));
	else if (e.getTracelevel() == Util::TraceLevel::L4_warning)
		ui->textEdit_Console->setTextColor(QColor::fromRgb(128, 0, 0));
	else {
        unsigned short grayVal = 20 * (static_cast<unsigned int> (e.getTracelevel()) - 5);
		ui->textEdit_Console->setTextColor(QColor::fromRgb(grayVal, grayVal, grayVal));
	}
	ui->textEdit_Console->append(QString::fromStdString(e.getText()));
    ui->textEdit_Console->moveCursor(QTextCursor::MoveOperation::End, QTextCursor::MoveMode::MoveAnchor);
    QCoreApplication::processEvents();
}

void MainWindow::_simulatorTraceErrorHandler(TraceErrorEvent e) {
	ui->textEdit_Console->setTextColor(QColor::fromRgb(255, 0, 0));
	ui->textEdit_Console->append(QString::fromStdString(e.getText()));
    QCoreApplication::processEvents();
}

void MainWindow::_simulatorTraceSimulationHandler(TraceSimulationEvent e) {
    if (e.getText().find("Event {time=")!= std::string::npos){
        ui->textEdit_Simulation->setTextColor(QColor::fromRgb(0,0,128));
    } else {
        unsigned short grayVal = 20 * (static_cast<unsigned int> (e.getTracelevel()) - 5);
        ui->textEdit_Simulation->setTextColor(QColor::fromRgb(grayVal, grayVal, grayVal));
    }
    ui->textEdit_Simulation->append(QString::fromStdString(e.getText()));
    QCoreApplication::processEvents();
}

void MainWindow::_simulatorTraceReportsHandler(TraceEvent e) {
	ui->textEdit_Reports->append(QString::fromStdString(e.getText()));
    QCoreApplication::processEvents();
}

void MainWindow::_onReplicationStartHandler(SimulationEvent* re){
    ModelSimulation* sim = simulator->getModels()->current()->getSimulation();
    QString text = QString::fromStdString(std::to_string(sim->getCurrentReplicationNumber())) + "/" + QString::fromStdString(std::to_string(sim->getNumberOfReplications()));
    ui->label_ReplicationNum->setText(text);
    QCoreApplication::processEvents();
}

void MainWindow::_onSimulationStartHandler(SimulationEvent* re) {
    _actualizeWidgets();
    ui->progressBarSimulation->setMaximum(simulator->getModels()->current()->getSimulation()->getReplicationLength());
    ui->tabWidgetModel->setCurrentIndex(1);
    QCoreApplication::processEvents();
}

void MainWindow::_onSimulationPausedHandler(SimulationEvent* re) {
	_actualizeWidgets();
    QCoreApplication::processEvents();
}

void MainWindow::_onSimulationResumeHandler(SimulationEvent* re) {
    _actualizeWidgets();
    ui->tabWidgetModel->setCurrentIndex(1);
    QCoreApplication::processEvents();
}

void MainWindow::_onSimulationEndHandler(SimulationEvent* re) {
	_actualizeWidgets();
	ui->tabWidgetModel->setCurrentIndex(2);
    QCoreApplication::processEvents();
}

void MainWindow::_onProcessEventHandler(SimulationEvent* re) {
    ui->progressBarSimulation->setValue(simulator->getModels()->current()->getSimulation()->getSimulatedTime());
    QCoreApplication::processEvents();
}

void MainWindow::_setOnEventHandlers() {
    simulator->getModels()->current()->getOnEvents()->addOnReplicationStartHandler(this, &MainWindow::_onReplicationStartHandler);
    simulator->getModels()->current()->getOnEvents()->addOnSimulationStartHandler(this, &MainWindow::_onSimulationStartHandler);
    simulator->getModels()->current()->getOnEvents()->addOnSimulationEndHandler(this, &MainWindow::_onSimulationEndHandler);
    simulator->getModels()->current()->getOnEvents()->addOnSimulationPausedHandler(this, &MainWindow::_onSimulationPausedHandler);
    simulator->getModels()->current()->getOnEvents()->addOnSimulationResumeHandler(this, &MainWindow::_onSimulationResumeHandler);
    simulator->getModels()->current()->getOnEvents()->addOnProcessEventHandler(this, &MainWindow::_onProcessEventHandler);
}

//-------------------------
// Simulator Fake Plugins
//-------------------------

void MainWindow::_insertPluginUI(Plugin* plugin) {
	if (plugin != nullptr) {
		if (plugin->isIsValidPlugin()) {
			QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(plugin->getPluginInfo()->getPluginTypename()));
            //QTreeWidgetItem *treeItem = new QTreeWidgetItem; //(ui->treeWidget_Plugins);
            std::string plugtextAdds = "["+plugin->getPluginInfo()->getCategory() + "]: ";
            QBrush brush;
			if (plugin->getPluginInfo()->isComponent()) {
                plugtextAdds += " Component";
                brush.setColor(Qt::white);
                item->setBackground(brush);
                item->setBackgroundColor(Qt::white);
                item->setIcon(QIcon(":/icons3/resources/icons/pack3/ico/component.ico"));
			} else {
                plugtextAdds += " DataDefinition";
                brush.setColor(Qt::lightGray);
                item->setBackground(brush);
                item->setBackgroundColor(Qt::lightGray);
                item->setIcon(QIcon(":/icons3/resources/icons/pack3/ico/calendar.ico"));
				//item->setFont(QFont::Style::StyleItalic);
			}
			if (plugin->getPluginInfo()->isSink()) {
                plugtextAdds += ", Sink";
                item->setTextColor(Qt::blue);
                item->setIcon(QIcon(":/icons3/resources/icons/pack3/ico/loadinv.ico"));
			}
			if (plugin->getPluginInfo()->isSource()) {
                plugtextAdds += ", Source";
                item->setTextColor(Qt::blue);
                item->setIcon(QIcon(":/icons3/resources/icons/pack3/ico/load.ico"));
			}
			if (plugin->getPluginInfo()->isReceiveTransfer()) {
                plugtextAdds += ", ReceiveTransfer";
                item->setTextColor(Qt::blue);
                item->setIcon(QIcon(":/icons3/resources/icons/pack3/ico/load.ico"));
			}
			if (plugin->getPluginInfo()->isSendTransfer()) {
                plugtextAdds += ", SendTransfer";
                item->setTextColor(Qt::blue);
                item->setIcon(QIcon(":/icons3/resources/icons/pack3/ico/loadinv.ico"));
			}
            //treeItem->setText(0,QString::fromStdString(plugtextAdds));
            plugtextAdds += "\n\nDescrption: " + plugin->getPluginInfo()->getDescriptionHelp();
            plugtextAdds += "\n\nTemplate: " + plugin->getPluginInfo()->getLanguageTemplate()+ " (double click to add to model)";
            //item->button...
            item->setToolTip(QString::fromStdString(plugtextAdds));
            item->setStatusTip(QString::fromStdString(plugin->getPluginInfo()->getLanguageTemplate()));
            ui->listWidget_Plugins->addItem(item);


		}
	}
}

void MainWindow::_insertFakePlugins() {
	PluginManager* pm = simulator->getPlugins();
    // TRYING SOME NEW ORGANIZATION (BASED ON ARENA 16..20)
    // ...
	//-----------------------------------------------------

	// OLD ORGANIZATION

	// model components
	// arena basic process
    (pm->insert("create.so"));
    (pm->insert("dispose.so"));
    (pm->insert("decide.so"));
    (pm->insert("batch.so"));
    (pm->insert("separate.so"));
    (pm->insert("assign.so"));
    (pm->insert("record.so"));
    (pm->insert("process.so"));
    (pm->insert("submodel.so"));
    (pm->insert("entitygroup.so"));
    (pm->insert("queue.so"));
    (pm->insert("set.so"));
    (pm->insert("resource.so"));
    (pm->insert("variable.so"));
    (pm->insert("schedule.so"));
    (pm->insert("entitygroup.so"));
	// arena advanced process
    (pm->insert("delay.so"));
    (pm->insert("dropoff.so"));
    (pm->insert("hold.so"));
    (pm->insert("match.so"));
    (pm->insert("pickup.so"));
    (pm->insert("read.so"));
    (pm->insert("write.so"));
    (pm->insert("release.so"));
    (pm->insert("remove.so"));
    (pm->insert("seize.so"));
    (pm->insert("search.so"));
    (pm->insert("signal.so"));
    (pm->insert("store.so"));
    (pm->insert("unstore.so"));
    (pm->insert("expression.so"));
    (pm->insert("failure.so"));
    (pm->insert("file.so"));
    (pm->insert("storage.so"));
	// arena transfer station
    (pm->insert("enter.so"));
    (pm->insert("leave.so"));
    (pm->insert("pickstation.so"));
    (pm->insert("route.so"));
    (pm->insert("sequence.so"));
    (pm->insert("station.so"));
    (pm->insert("label.so"));
	// arena transfer conveyour
    (pm->insert("access.so"));
    (pm->insert("exit.so"));
    (pm->insert("start.so"));
    (pm->insert("stop.so"));
    (pm->insert("conveyour.so"));
    (pm->insert("segment.so"));
	// arena transfer transport
    (pm->insert("alocate.so"));
    (pm->insert("free.so"));
    (pm->insert("halt.so"));
    (pm->insert("move.so"));
    (pm->insert("request.so"));
    (pm->insert("transporter.so"));
    (pm->insert("distance.so"));
    (pm->insert("network.so"));
    (pm->insert("networklink.so"));
	// others
    (pm->insert("dummy.so"));
    (pm->insert("lsode.so"));
    (pm->insert("biochemical.so"));
    (pm->insert("markovchain.so"));
    (pm->insert("cellularautomata.so"));
    (pm->insert("cppforg.so"));
    // now complete the information
    simulator->getPlugins()->completePluginsFieldsAndTemplates();
    for (unsigned int i=0; i<simulator->getPlugins()->size(); i++) {
        //@TODO: now it's the opportunity to adjust template
        _insertPluginUI(simulator->getPlugins()->getAtRank(i));
    }
}


//-------------------------
// PRIVATE SLOTS
//-------------------------

void MainWindow::on_actionNew_triggered() {
	Model* m;
	if ((m = simulator->getModels()->current()) != nullptr) {
		QMessageBox::StandardButton reply = QMessageBox::question(this, "New Model", "There is a model oppened already. Do you want to remove it and to create new model?", QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No) {
			return;
		}
	}
    _insertCommandInConsole("new");
	if (m != nullptr) {
		simulator->getModels()->remove(m);
	}
	m = simulator->getModels()->newModel();
	ui->textEdit_Model->clear();
	ui->textEdit_Simulation->clear();
	ui->textEdit_Reports->clear();
	ui->textEdit_Console->moveCursor(QTextCursor::End);
    ui->tabWidgetModel->setCurrentIndex(0);
	// create a basic initial template for the model
	std::string tempFilename = "./temp.tmp";
	m->getPersistence()->setOption(ModelPersistence_if::Options::SAVEDEFAULTS, true);
	bool res = m->save(tempFilename);
	m->getPersistence()->setOption(ModelPersistence_if::Options::SAVEDEFAULTS, false);
	if (res) { // read the file saved and copy its contents to the model text editor
		std::string line;
		std::ifstream file(tempFilename);
		if (file.is_open()) {
            ui->textEdit_Model->append("# Genesys Model File");
            ui->textEdit_Model->append("# Simulator, ModelInfo and ModelSimulation");
            while (std::getline(file, line)) {
				ui->textEdit_Model->append(QString::fromStdString(line));
			}
			file.close();
            ui->textEdit_Model->append("\n# Model Data Definitions");
            ui->textEdit_Model->append("\n\n# Model Components\n");
			QMessageBox::information(this, "New Model", "Model successfully created");
		} else {
			ui->textEdit_Console->append(QString("Error reading template model file"));
		}
		_actualizeModelTextHasChanged(true);
		_setOnEventHandlers();
	} else {
		ui->textEdit_Console->append(QString("Error saving template model file"));
	}
	_actualizeWidgets();
}

void MainWindow::on_actionSave_triggered() {
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save Model"), "",
			tr("Genesys Model (*.gen);;All Files (*)"));
	if (fileName.isEmpty())
		return;
	else {
        _insertCommandInConsole("save "+fileName.toStdString());
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly)) {
			QMessageBox::information(this, tr("Unable to access file to save"),
					file.errorString());
			return;
		}
        std::ofstream savefile;
        savefile.open(fileName.toStdString(), std::ofstream::out);
        QString data = ui->textEdit_Model->toPlainText();
        QStringList strList = data.split(QRegExp("[\n]"), QString::SkipEmptyParts);
        for (unsigned int i = 0; i < strList.size(); i++) {
            savefile << strList.at(i).toStdString() << std::endl;
        }
        savefile.close();
        QMessageBox::information(this, "Save Model", "Model successfully saved");
		_actualizeModelTextHasChanged(false);
	}
	_actualizeWidgets();
}

void MainWindow::on_actionClose_triggered() {
    _insertCommandInConsole("close");
	simulator->getModels()->remove(simulator->getModels()->current());
	_actualizeWidgets();
	QMessageBox::information(this, "Close Model", "Model successfully closed");
}

void MainWindow::on_actionExit_triggered() {
    QMessageBox::StandardButton res;
    if (this->_textModelHasChanged) {
        res = QMessageBox::question(this, "Exit GenESyS", "Model has changed. Do you want to save it?", QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::Yes) {
            this->on_actionSave_triggered();
        }
    }
    res = QMessageBox::question(this, "Exit GenESyS", "Do you want to exit GenESyS?", QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
		QApplication::quit();
    } else {
        // it does not quit, but the window is closed. Check it. @TODO
    }
}

void MainWindow::on_actionStop_triggered() {
	_insertCommandInConsole("stop");
	simulator->getModels()->current()->getSimulation()->stop();
}

void MainWindow::on_actionStart_triggered() {
	_insertCommandInConsole("start");
    if (_checkStartSimulation())
        simulator->getModels()->current()->getSimulation()->start();
}

void MainWindow::on_actionStep_triggered() {
	_insertCommandInConsole("step");
    if (_checkStartSimulation())
        simulator->getModels()->current()->getSimulation()->step();
}

void MainWindow::on_actionPause_triggered() {
	_insertCommandInConsole("pause");
	simulator->getModels()->current()->getSimulation()->pause();
}

void MainWindow::on_actionResume_triggered() {
    _insertCommandInConsole("resume");
    if (_checkStartSimulation())
        simulator->getModels()->current()->getSimulation()->start();
}

void MainWindow::on_actionOpen_triggered() {
	QString fileName = QFileDialog::getOpenFileName(
			this, "Open Model", "./",
			tr("All files (*.*);;Genesys model (*.gen)"));
	if (fileName == "") {
		return;
	}
    _insertCommandInConsole("load "+fileName.toStdString());
	bool result = simulator->getModels()->loadModel(fileName.toStdString());
	if (result) {
		_clearModelEditors();
		std::string line;
		std::ifstream file(fileName.toStdString());
		if (file.is_open()) {
			while (std::getline(file, line)) {
				ui->textEdit_Model->append(QString::fromStdString(line));
			}
			file.close();
		} else {
			ui->textEdit_Console->append(QString("Error reading model file"));
		}
		ui->textEdit_Console->append("\n");
		_setOnEventHandlers();
		_actualizeModelTextHasChanged(false);
		_actualizeWidgets();
        ui->tabWidgetModel->setCurrentIndex(0);
		QMessageBox::information(this, "Open Model", "Model successfully oppened");
	} else {
		QMessageBox::warning(this, "Open Model", "Error while opening model");
	}
	_actualizeWidgets();
}

void MainWindow::on_textEdit_Model_textChanged() {
	this->_actualizeModelTextHasChanged(true);
}

void MainWindow::on_listWidget_Plugins_itemDoubleClicked(QListWidgetItem *item)
{
    if (ui->textEdit_Model->isEnabled()) {
        if (item->toolTip().contains("DataDefinition")) {
            //QTextDocument::FindFlags flag;
            QTextCursor cursor = ui->textEdit_Model->textCursor();
            QTextCursor cursorSaved = cursor;
            cursor.movePosition(QTextCursor::Start);
            ui->textEdit_Model->setTextCursor(cursor);
            //QTextCursor highlightCursor(ui->textEdit_Model->document());
            if (ui->textEdit_Model->find("# Model Components")) {
                ui->textEdit_Model->moveCursor(QTextCursor::MoveOperation::Left, QTextCursor::MoveMode::MoveAnchor);
                ui->textEdit_Model->moveCursor(QTextCursor::MoveOperation::Up, QTextCursor::MoveMode::MoveAnchor);
                ui->textEdit_Model->insertPlainText(item->statusTip()+"\n");
            } else {
                ui->textEdit_Model->append(item->statusTip());
            }
        } else {
            ui->textEdit_Model->append(item->statusTip());
        }
    }
}


void MainWindow::on_listWidget_Plugins_doubleClicked(const QModelIndex &index)
{
    //
}

void MainWindow::on_listWidget_Plugins_itemClicked(QListWidgetItem *item)
{
    //
}

void MainWindow::on_listWidget_Plugins_clicked(const QModelIndex &index)
{
    //
}

void MainWindow::on_actionCheck_triggered()
{
    _insertCommandInConsole("check");
    bool res = simulator->getModels()->current()->check();
    if (res) {
        QMessageBox::information(this, "Model Check", "Model successfully checked.");
    } else {
        QMessageBox::critical(this, "Model Check", "Model has erros. See the console for more information.");
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About Genesys", "Genesys is a result of teaching and research activities of Professor Dr. Ing Rafael Luiz Cancian. \nIt began in early 2002 as a way to teach students the basics and simulation techniques of systems implemented by other comercial simulation tools, such as Arena. \nIn Genesys development he replicated all the SIMAN language, used by Arena software, and Genesys has become a clone of that tool, including its graphical interface. \nGenesys allowed the inclusion of new simulation components through dynamic link libraries and also the parallel execution of simulation models in a distributed environment. \nThe development of Genesys continued until 2007, when the professor stopped teaching systems simulation classes. \nTen years later the professor starts again to teach systems simulation classes and to carry out scientific research in the area. \nSo in 2018 Genesys is reborn, with new language and programming techniques, and even more ambitious goals.");
}
