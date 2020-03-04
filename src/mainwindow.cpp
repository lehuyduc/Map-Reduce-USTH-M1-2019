#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logconsole.h"
#include "ui_logconsole.h"
#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QVector>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)    
    , resource(1)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lblMatSize->setVisible(false);
    ui->lEditMatSize->setVisible(false);

    console = new LogConsole();
    console->setModal(false);
    console->show();
    console->activateWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//-------------------   THREAD RESULTS HANDLER
//https://doc.qt.io/qt-5/qthread.html
void MainWindow::slotGenerateArrayFinish(const vector<double> &arr) {
    numData = arr.size();
    arrData = arr;
    createDataThread.quit();
    createDataThread.wait();
    QMessageBox::information(this, "Success", "Generate array data successful");
    console->getUI()->txtBrowserLog->append("Generate array data successful");
    slotUpdateProgress(100);
    resource.release(1);    
}

void MainWindow::slotArrayExperimentFinish(const vector<Result> &res) {
    results = res;
    experimentThread.quit();
    experimentThread.wait();
    QMessageBox::information(this, "Success", "Array experiment successful");
    console->getUI()->txtBrowserLog->append("Array experiment successful");

    try {
            utils::outputFile("arrayResultAutosave.txt",results);
        }
        catch (std::exception ex) {
            qDebug() << "Array experiment output: " << ex.what() << "\n";
            QMessageBox::information(this, "Error", "Auto-save array results to file failed\n");
            console->getUI()->txtBrowserLog->append("Auto-save array results to file failed");
            return;
        }
    QMessageBox::information(this, "Success", "Auto-save array results to file successful");
    console->getUI()->txtBrowserLog->append("Auto-save array results to file successful");
    slotUpdateProgress(100);
    resource.release(1);
}

void MainWindow::slotGenerateMatrixFinish(const vector<Matrix<double> > &mats) {
    numData = mats.size();
    matData = mats;
    createDataThread.quit();
    createDataThread.wait();
    QMessageBox::information(this, "Success", "Generate matrix data successful");
    console->getUI()->txtBrowserLog->append("Generate matrix data successful");
    slotUpdateProgress(100);
    resource.release(1);
}

void MainWindow::slotMatrixExperimentFinish(const vector<Result> &res) {
    results = res;
    experimentThread.quit();
    experimentThread.wait();
    QMessageBox::information(this, "Success", "Matrix experiment successful");
    console->getUI()->txtBrowserLog->append("Matrix experiment successful");

    try {
            utils::outputFile("matrixResultAutosave.txt",results);
        }
        catch (std::exception ex) {
            qDebug() << "Matrix experiment output: " << ex.what() << "\n";
            QMessageBox::information(this, "Error", "Auto-save matrix results to file failed\n");
            console->getUI()->txtBrowserLog->append("Auto-save matrix results to file failed");
            return;
        }
    QMessageBox::information(this, "Success", "Auto-save matrix results to file successful");
    console->getUI()->txtBrowserLog->append("Auto-save matrix results to file successful");
    slotUpdateProgress(100);
    resource.release(1);
}

void MainWindow::slotParseDistributionFinish(const Distribution &parsedDistribution) {
    distribution = parsedDistribution;
    createDistributionThread.quit();
    createDistributionThread.wait();
    if (distribution.valid())
    {
        QMessageBox::information(this, "Success", "Create distribution successful");
        console->getUI()->txtBrowserLog->append("Create distribution successful");
    }
    else {
        QMessageBox::information(this, "Error", "Distribution expression is invalid");
        console->getUI()->txtBrowserLog->append("Distribution expression is invalid");
    }
    slotUpdateProgress(100);
    resource.release(1);
}

void MainWindow::slotReceiveAlert(QString alert) {
    QMessageBox::information(this, "Error", alert);
}

void MainWindow::slotUpdateProgress(int value) {
    console->getUI()->progBar->setValue(value);
}

//-------------------   FUNCTIONS TO START AND RUN NEW THREADS
// https://doc.qt.io/qt-5/qthread.html
bool MainWindow::threadGenerateArray() {
    slotUpdateProgress(0);
    if (numData <= 0)
        throw MainWindowException("MainWindow generateArray: numData <= 0");
    if (!distribution.valid())
        throw MainWindowException("MainWindow generateArray: distribution invalid");

    if (!resource.tryAcquire()) return false;

    ArrayGenerator *arrGen = new ArrayGenerator(distribution);
    arrGen->moveToThread(&createDataThread);
    connect(&createDataThread, &QThread::finished, arrGen, &QObject::deleteLater);
    connect(this, &MainWindow::signalGenerateArray, arrGen, &ArrayGenerator::slotGenerateArray);
    connect(arrGen, &ArrayGenerator::signalGenerateFinish, this, &MainWindow::slotGenerateArrayFinish);
    connect(arrGen, &ArrayGenerator::signalUpdateProgress, this, &MainWindow::slotUpdateProgress);

    createDataThread.start();

    emit signalGenerateArray(numData);

    return true;
}

bool MainWindow::threadRunArrayExperiment() {
    slotUpdateProgress(0);
    if (dataType!=ARRAY)
        throw MainWindowException("threadRunArrayExperiment: dataType != ARRAY");
    if (numData<=0)
        throw MainWindowException("threadRunArrayExperiment: numData<=0");
    if (!distribution.valid())
        throw MainWindowException("threadRunArrayExperiment: distribution invalid");

    if (!resource.tryAcquire()) return false;

    ArrayExperiment *arrExper = new ArrayExperiment(arrData, distribution);
    arrExper -> moveToThread(&experimentThread);
    connect(&experimentThread, &QThread::finished, arrExper, &QObject::deleteLater);
    connect(this, &MainWindow::signalArrayExperiment, arrExper, &ArrayExperiment::slotRunArrayExperiment);
    connect(arrExper, &ArrayExperiment::signalExperimentFinish, this, &MainWindow::slotArrayExperimentFinish);
    connect(arrExper, &ArrayExperiment::signalUpdateProgress, this, &MainWindow::slotUpdateProgress);

    experimentThread.start();

    emit signalArrayExperiment(operation, numTest, testAlgos, shuffle);

    return true;
}

bool MainWindow::threadGenerateMatrix() {
    slotUpdateProgress(0);
    if (numData <= 0)
        throw MainWindowException("threadGenerateMatrix: numData <= 0");
    if (matSize <= 0)
        throw MainWindowException("threadGenerateMatrix: matSize <= 0");
    if (!distribution.valid())
        throw MainWindowException("threadGenerateMatrix: distribution invalid");

    if (!resource.tryAcquire()) return false;

    MatrixGenerator *matGen = new MatrixGenerator(distribution);
    matGen->moveToThread(&createDataThread);
    connect(&createDataThread, &QThread::finished, matGen, &QObject::deleteLater);
    connect(this, &MainWindow::signalGenerateMatrix, matGen, &MatrixGenerator::slotGenerateMatrix);
    connect(matGen, &MatrixGenerator::signalGenerateFinish, this, &MainWindow::slotGenerateMatrixFinish);
    connect(matGen, &MatrixGenerator::signalUpdateProgress, this, &MainWindow::slotUpdateProgress);

    createDataThread.start();

    emit signalGenerateMatrix(numData, matSize);

    return true;
}

bool MainWindow::threadRunMatrixExperiment() {
    slotUpdateProgress(0);
    if (dataType!=MATRIX)
        throw MainWindowException("threadRunMatrixExperiment: dataType != MATRIX");
    if (numData<=0)
        throw MainWindowException("threadRunMatrixExperiment: numData<=0");
    if (matSize<=0)
        throw MainWindowException("threadRunMatrixExperiment: numData<=0");
    if (!distribution.valid())
        throw MainWindowException("threadRunMatrixExperiment: distribution invalid");

    if (!resource.tryAcquire()) return false;

    MatrixExperiment *matExper = new MatrixExperiment(matData, distribution);
    matExper -> moveToThread(&experimentThread);
    connect(&experimentThread, &QThread::finished, matExper, &QObject::deleteLater);
    connect(this, &MainWindow::signalMatrixExperiment, matExper, &MatrixExperiment::slotRunMatrixExperiment);
    connect(matExper, &MatrixExperiment::signalExperimentFinish, this, &MainWindow::slotMatrixExperimentFinish);
    connect(matExper, &MatrixExperiment::signalUpdateProgress, this, &MainWindow::slotUpdateProgress);

    experimentThread.start();

    emit signalMatrixExperiment(operation, numTest, testAlgos, shuffle);

    return true;
}

bool MainWindow::threadParseDistribution() {
    slotUpdateProgress(0);

    if (!resource.tryAcquire()) return false;

    Parser *parser = new Parser(binNumber, lowerBound, upperBound);
    parser->moveToThread(&createDistributionThread);
    connect(&createDistributionThread, &QThread::finished, parser, &QObject::deleteLater);
    connect(this, &MainWindow::signalParseDistribution, parser, &Parser::slotParseDistribution);
    connect(parser, &Parser::signalParseFinish, this, &MainWindow::slotParseDistributionFinish);
    connect(parser, &Parser::signalUpdateProgress, this, &MainWindow::slotUpdateProgress);

    createDistributionThread.start();

    emit signalParseDistribution(getDistributionString());

    return true;
}


//--------------------  ON-CLICK LISTENER

void MainWindow::on_cBoxDataType_currentIndexChanged(int index)
{
    ui->cBoxOperation->setCurrentIndex(0);

    int count = ui->cBoxOperation->count();

    qDebug() << "cBox data type count " << count << "\n";

    if (index == 0)
    {
        if (count > 2) ui->cBoxOperation->removeItem(2);
        ui->lblMatSize->setVisible(false);
        ui->lEditMatSize->setVisible(false);
    }
    if (index == 1)
    {
        ui->lEditMatSize->clear();
        if (count < 3) ui->cBoxOperation->insertItem(2, "Matmul");
        ui->lblMatSize->setVisible(true);
        ui->lEditMatSize->setVisible(true);
    }
}

void MainWindow::on_pButtonOpenFile_1_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait.");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait.");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "C://",
                "All files (*.*);;Text File (*.txt)");

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "Error", "File cannot be opened");
        console->getUI()->txtBrowserLog->append("File cannot be opened");
        return;
    }

    QTextStream in(&file);

    ui->txtBrowser_1->setText(in.readAll());

    file.close();
}

QString MainWindow::getDistributionString() {
    /* Distribution Tab */
    if (ui->tabDistribution->currentIndex() == 0)
    {
        QString U_str = "";
        QString N_str = "";
        QString Exp_str = "";
        QString G_str = "";

        if (ui->lEditParaU_a->text().size() != 0 && ui->lEditParaU_b->text().size() != 0)
            U_str.append("U(" + ui->lEditParaU_a->text() + "," + ui->lEditParaU_b->text() + ")");

        if (ui->lEditParaN_mean->text().size() != 0 && ui->lEditParaN_var->text().size() != 0)
            N_str.append("N(" + ui->lEditParaN_mean->text() + "," + ui->lEditParaN_var->text() + ")");

        if (ui->lEditParaExp_lambda->text().size() != 0)
            Exp_str.append("E(" + ui->lEditParaExp_lambda->text() + ")");

        if (ui->lEditParaG_alpha->text().size() != 0 && ui->lEditParaG_lambda->text().size() != 0)
            G_str.append("G(" + ui->lEditParaG_alpha->text() + "," + ui->lEditParaG_lambda->text() + ")");

        QStringList dist_list = (QStringList() << U_str << N_str << Exp_str << G_str);
        dist_list.removeAll("");

        return dist_list.join(" + ");
    }

    if (ui->tabDistribution->currentIndex() == 1)
    {
        return ui->lEditEquation->text();
    }

    if (ui->tabDistribution->currentIndex() == 2)
    {
        return ui->txtBrowser_1->toPlainText();
    }

    return "";
}


vector<double> MainWindow::getDistributionParams() {
    vector<double> res;
    QString binNumberStr = ui->lEditBinNum->text();
    QString lowerBoundStr = ui->lEditLowerBound->text();
    QString upperBoundStr = ui->lEditUpperBound->text();

    bool valid1, valid2, valid3;
    long long binNumber = utils::str2double(binNumberStr, valid1);// (binNumberStr.toDouble(&valid1));
    double lowerBound = utils::str2double(lowerBoundStr, valid2); //lowerBoundStr.toDouble(&valid2);
    double upperBound = utils::str2double(upperBoundStr, valid3);//upperBoundStr.toDouble(&valid3);

    if (!valid1 || !valid2 || !valid3) return res; // return empty vector

    res.push_back(binNumber);
    res.push_back(lowerBound);
    res.push_back(upperBound);
    return res;
}

void MainWindow::on_pButtonCreateDistribution_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait.");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait.");

        return;
    }

    vector<double> distributionParams = getDistributionParams();
    if (distributionParams.size() != 3) {
        QMessageBox::information(this, "Error", "Invalid distribution parameters");
        console->getUI()->txtBrowserLog->append("Invalid distribution parameters");
        return;
    }

    if (!Parser::validParams(distributionParams[0], distributionParams[1], distributionParams[2])) {
        QMessageBox::information(this, "Error", "Invalid distribution parameters values");
        console->getUI()->txtBrowserLog->append("Invalid distribution parameters values");
        return;
    }

    binNumber = distributionParams[0];
    lowerBound = distributionParams[1];
    upperBound = distributionParams[2];
qDebug() << " Reached here\n";
    if (!threadParseDistribution()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait and try again");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait and try again");
        return;
    }
    else {
        QMessageBox::information(this, "Update", "Creating distribution is in progress");
        console->getUI()->txtBrowserLog->append("Creating distribution is in progress");
    }
}


void MainWindow::on_pButtonGen_clicked()
{
    qDebug() << "buttonGen clicked \n";
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait.");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait.");
        return;
    }

    QString numDataStr = ui->lEditNumData->text();
    bool validNumData = false;
    numData = utils::str2double(numDataStr, validNumData); //numDataStr.toDouble(&validNumData);
    if (!validNumData || numData <= 0) {
        QMessageBox::information(this, "Error", "Invalid number of data element");
        console->getUI()->txtBrowserLog->append("Invalid number of data element");
        return;
    }

    if (!distribution.valid()) {
        QMessageBox::information(this, "Error", "Please create a distribution first");
        console->getUI()->txtBrowserLog->append("Please create a distribution first");
        return;
    }

    QString dataTypeStr = ui->cBoxDataType->currentText();
    if (dataTypeStr=="Matrix") {
        QString matSizeStr = ui->lEditMatSize->text();
        bool validMatSize;
        matSize = utils::str2double(matSizeStr, validMatSize); //matSizeStr.toDouble(&validMatSize);
        if (!validMatSize || matSize <= 0) {
            QMessageBox::information(this, "Error", "Invalid matrix size");
            console->getUI()->txtBrowserLog->append("Invalid matrix size");
            return;
        }
    }


    if (dataTypeStr=="Array") {
        dataType = ARRAY;
        ArrayGenerator arrayGenerator(distribution);

        if (!threadGenerateArray()) {
            QMessageBox::information(this, "Error", "Generate failed. Please try again");
            console->getUI()->txtBrowserLog->append("Generate failed. Please try again");
            return;

        }
        QMessageBox::information(this, "Update", "Generate array is in progress");
        console->getUI()->txtBrowserLog->append("Generate array is in progress");
    }
    else if (dataTypeStr=="Matrix") {
        dataType = MATRIX;
        MatrixGenerator matrixGenerator(distribution);
        if (!threadGenerateMatrix()) {
            QMessageBox::information(this, "Error", "Generate failed. Please try again");
            console->getUI()->txtBrowserLog->append("Generate failed. Please try again");
            return;
        }
        QMessageBox::information(this, "Update", "Generate array is in progress");
        console->getUI()->txtBrowserLog->append("Generate array is in progress");
    }
}

void MainWindow::on_pButtonSaveDataset_clicked()
{
    qDebug() << "buttonSaveDataset clicked\n";
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait");
        return;
    }

    if (arrData.size() == 0 && matData.size() == 0) {
        QMessageBox::information(this, "Error", "No dataset to save");
        return;
    }

    if (ui->lEditSaveDir->text() == "")
    {
        QMessageBox::information(this, "Error", "Save Directory is NOT set!");
        console->getUI()->txtBrowserLog->append("Save Directory is NOT set!");
        return;
    }
    else
    {        
        // Save to Dir
        QDateTime now = QDateTime::currentDateTime();
        QString format = now.toString("dd.MMM.yyyy-hhmmss");
        QString savefile = ui->lEditSaveDir->text() + format + ".txt";

        if (dataType==ARRAY) {
            if (arrData.size()==0) {
                QMessageBox::information(this, "Error", "No array dataset to save");
                return;
            }
            console->getUI()->txtBrowserLog->append("Start saving array dataset.");

            QString savefile = ui->lEditSaveDir->text() + "/" + format + ".array";
            bool fileSaveSuccess = utils::saveArray(savefile, arrData, 12);
            if (!fileSaveSuccess) {
                QMessageBox::information(this, "Error", "Can't save array to file. Please try again.");
                console->getUI()->txtBrowserLog->append("Can't save array to file. Please try again.");
            }
            else {
                QMessageBox::information(this, "Success", "File save array successful");
                console->getUI()->txtBrowserLog->append("File save array successful");
            }
        }
        else {            
            if (matData.size()==0) {
                QMessageBox::information(this, "Error", "No matrix dataset to save!");
                return;
            }
            console->getUI()->txtBrowserLog->append("Start saving matrix dataset.");

            QString savefile = ui->lEditSaveDir->text() + "/" + format + ".matrix";
            bool fileSaveSuccess = utils::saveMatrix(savefile, matData, 12);
            if (!fileSaveSuccess) {
                QMessageBox::information(this, "Error", "Can't save matrix file. Please try again.");
                console->getUI()->txtBrowserLog->append("Can't save matrix file. Please try again.");
            }
            else {
                QMessageBox::information(this, "Success", "File save matrix successful");
                console->getUI()->txtBrowserLog->append("File save matrix successful");
            }
        }
    }

}

void MainWindow::on_pButtonOpenFile_2_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "C://",
                "All files (*.*);;Text File (*.txt)");

    if (!filename.contains(".array") && !filename.contains(".matrix")) {
        QMessageBox::information(this, "Error", "Invalid file format. Must be .array or .matrix file");
        console->getUI()->txtBrowserLog->append("Invalid file format. Must be .array or .matrix file");
        ui->txtBrowser_2->setText("Invalid file format");
        return;
    }

    QString dataTypeStr = ui->cBoxDataType->currentText();
    if (dataTypeStr=="Array") dataType = ARRAY;
    else if (dataTypeStr=="Matrix") dataType = MATRIX;

    if ((dataType==ARRAY && filename.contains(".matrix"))
            || (dataType==MATRIX && filename.contains(".array"))) {
        QMessageBox::information(this, "Error", "File has wrong data type");
        console->getUI()->txtBrowserLog->append("File has wrong data type");
        ui->txtBrowser_2->setText("File has wrong data type");
        return;
    }

    std::ifstream fin;

    fin = std::ifstream(filename.toStdString().c_str());
    if (!fin.is_open()) {
        QMessageBox::information(this, "Error", "File cannot be opened");
        console->getUI()->txtBrowserLog->append("File cannot be opened");
        ui->txtBrowser_2->setText("File cannot be opened");
        return;
    }

    ui->txtBrowser_2->setText("Importing file...");
    try {
        if (dataType==ARRAY) {
            console->getUI()->txtBrowserLog->append("Start loading array dataset.");
            fin >> numData;
            arrData.clear();
            for (unsigned i=0; i<numData; i++) {
                double val;
                fin >> val;
                arrData.push_back(val);
            }
            fin.close();
        }
        else if (dataType==MATRIX) {
            console->getUI()->txtBrowserLog->append("Start loading matrix dataset.");
            fin >> numData >> matSize >> matSize;
            matData.clear();
            Matrix<double> currentMat(matSize, matSize);
            for (unsigned i=0; i<numData; i++) {
                for (unsigned t=0; t<matSize*matSize; t++) fin >> currentMat[i];
                matData.push_back(currentMat);
            }
            fin.close();
        }
    }
    catch (...) {
        QMessageBox::information(this, "Error", "File cannot be read or corrupted");
        console->getUI()->txtBrowserLog->append("File cannot be read or corrupted");
        ui->txtBrowser_2->setText("File cannot be read or corrupted");
        fin.close();
        return;
    }

    ui->txtBrowser_2->setText("Data imported successfully!");
}


void MainWindow::on_pButtonBrowseDir_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait.");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait.");
        return;
    }

    QString folderDir = QFileDialog::getExistingDirectory(
                this,
                tr("Open Directory"),
                qApp->applicationDirPath(),
                QFileDialog::ShowDirsOnly);

    ui->lEditSaveDir->setText(folderDir);
}

void MainWindow::on_pButtonBrowseSave_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait.");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait.");
        return;
    }

    QString folderDir = QFileDialog::getExistingDirectory(
                this,
                tr("Open Directory"),
                qApp->applicationDirPath(),
                QFileDialog::ShowDirsOnly);

    ui->lEditSaveDir_2->setText(folderDir);
}

void MainWindow::on_gBoxAlgorithm_clicked()
{

}

void MainWindow::on_pButtonRun_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Another task is in progress. Please wait.");
        console->getUI()->txtBrowserLog->append("Another task is in progress. Please wait.");
        return;
    }

    ui->outputText->clear();

    if (numData <= 0) {
        QMessageBox::information(this, "Error", "Number of data element <= 0");
        console->getUI()->txtBrowserLog->append("Number of data element <= 0");
        return;
    }

    if (!distribution.valid()) {
        QMessageBox::information(this, "Error", "Please create a distribution first");
        console->getUI()->txtBrowserLog->append("Please create a distribution first");
        return;
    }
    qDebug() << "after distribution";

    QString numTestStr = ui->lEditNumTest->text();
    bool validNumTest;
    numTest = utils::str2double(numTestStr, validNumTest); //numTestStr.toDouble(&validNumTest);
    if (!validNumTest || numTest <= 0) {
        QMessageBox::information(this, "Error", "Invalid number of test");
        console->getUI()->txtBrowserLog->append("Invalid number of test");
        return;
    }

    if (ui->chkBoxGenNewData->isChecked()) shuffle = false; else shuffle = true;

    testAlgos.clear();
    if (ui->chkBoxLinear->isChecked()) testAlgos.push_back(LINEAR);
    if (ui->chkBoxSplitMerge->isChecked()) testAlgos.push_back(SPLIT_MERGE);
    if (ui->chkBoxSortLinear->isChecked()) testAlgos.push_back(SORT);
    if (ui->chkBoxSortAppend->isChecked()) testAlgos.push_back(SORT_APPEND);

    if (testAlgos.empty()) {
        QMessageBox::information(this, "Error", "Please select some algorithms");
        console->getUI()->txtBrowserLog->append("Please select some algorithms");
        return;
    }
    qDebug() << "after testAlgos";

    QString operationStr = ui->cBoxOperation->currentText();
    if (operationStr=="Sum") operation = ADD;
    else if (operationStr=="Multiplication") operation = MUL;
    else if (operationStr=="Matmul") operation = MATMUL;

    qDebug() << "after operation";

    results.clear();

    if (dataType==ARRAY) {
        threadRunArrayExperiment();
        QMessageBox::information(this, "Update", "Array experiment is in progress");
        console->getUI()->txtBrowserLog->append("Array experiment is in progress");
    }
    else {
        threadRunMatrixExperiment();
        QMessageBox::information(this, "Update", "Matrix experiment is in progress");
        console->getUI()->txtBrowserLog->append("Matrix experiment is in progress");
    }
}

void MainWindow::on_pButtonSaveResult_clicked()
{
    if (!resource.available()) {
        QMessageBox::information(this, "Error", "Auto-save matrix results to file failed\n");
        console->getUI()->txtBrowserLog->append("Auto-save matrix results to file failed");
    }

    if (results.size() <= 2) {
        QMessageBox::information(this, "Error", "There is no result to save");
        console->getUI()->txtBrowserLog->append("There is no result to save");
        return;
    }

    QString folder = ui->lEditSaveDir_2->text();

    QDateTime now = QDateTime::currentDateTime();
    QString format = now.toString("dd.MMM.yyyy-hhmmss");
    QString savefile = folder + "/result" + format + ".csv";

    try {
            console->getUI()->txtBrowserLog->append("Start saving result.");
            utils::outputFile(savefile,results);
        }
        catch (std::exception ex) {
            qDebug() << "Matrix experiment output: " << ex.what() << "\n";
            QMessageBox::information(this, "Error", "Save current results to file failed");
            console->getUI()->txtBrowserLog->append("Save current results to file failed");
            return;
        }
    QMessageBox::information(this, "Success", "Save current results to file " + savefile + " successful");
    console->getUI()->txtBrowserLog->append("Save current results to file " + savefile + " successful");
}

void MainWindow::on_pButtonLogConsole_clicked()
{
    console->setModal(false);
    console->show();
    console->activateWindow();
}
