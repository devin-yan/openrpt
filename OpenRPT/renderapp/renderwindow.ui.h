/*
 * Copyright (c) 2002-2005 by OpenMFG, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * If you do not wish to be bound by the terms of the GNU General Public
 * License, DO NOT USE THIS SOFTWARE.  Please contact OpenMFG, LLC at
 * info@openmfg.com for details on how to purchase a commercial license.
 */

void RenderWindow::init()
{

}


void RenderWindow::helpAbout()
{
  QMessageBox::about(this, tr("About %1").arg(_name),
    tr("%1 version %2"
       "\n\n%3 is a tool for printing report definition files against a database."
       "\n\n%4, All Rights Reserved").arg(_name).arg(_version).arg(_name).arg(_copyright));
}


void RenderWindow::fileOpen()
{
  QString filename = QFileDialog::getOpenFileName(_reportName->text(), tr("XML (*.xml);;All Files (*)"), this);
  if(filename.isEmpty())
    return;

  QDomDocument doc;
  QString errMsg;
  int errLine, errColm;
  QFile file(filename);
  if(!doc.setContent(&file, &errMsg, &errLine, &errColm))
  {
    QMessageBox::critical(this, tr("Error Loading File"),
      tr("There was an error opening the file %1."
         "\n\n%2 on line %3 column %4.")
         .arg(filename).arg(errMsg).arg(errLine).arg(errColm) );
    return;
  }

  QDomElement root = doc.documentElement();
  if(root.tagName() != "report")
  {
    QMessageBox::critical(this, tr("Not a Valid File"),
      tr("The file %1 does not appear to be a valid file."
         "\n\nThe root node is not 'report'.").arg(filename) );
    return;
  }

  _doc = doc;
  _report->setText(filename);
  _reportInfo->setEnabled(true);

  _reportName->setText(QString::null);
  _reportTitle->setText(QString::null);
  _reportDescription->setText(QString::null);
  for(QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
  {
    if(n.nodeName() == "name")
      _reportName->setText(n.firstChild().nodeValue());
    else if(n.nodeName() == "title")
      _reportTitle->setText(n.firstChild().nodeValue());
    else if(n.nodeName() == "description")
      _reportDescription->setText(n.firstChild().nodeValue());
  }
}


void RenderWindow::filePrint()
{
  orReport report;
  report.setDom(_doc);
  report.setParamList(getParameterList());

  if(report.isValid())
    report.print();
  else
    report.reportError(this);
}

void RenderWindow::fileExit()
{
  qApp->closeAllWindows();
}



void RenderWindow::sAdd()
{
  bool ok = false;
  bool active = false;

  QString name;
  QString varType;
  QVariant var;

  NewVariant newVar(this);

  while(!ok)
  {
    if(newVar.exec() != QDialog::Accepted)
      return;

    name = newVar._name->text();
    varType = newVar._type->currentText();

    ok = !_params.contains(name);
    if(!ok)
      QMessageBox::warning(this, tr("Name already exists"), tr("The name for the parameter you specified already exists in the list."));
  }


  BoolEdit * be = 0;
  IntEdit * ie = 0;
  DoubleEdit * de = 0;
  StringEdit * se = 0;
  ListEdit * le = 0;

  if(varType == NewVariant::tr("String")) {
    se = new StringEdit(this);
    se->_name->setText(name);
    ok = (se->exec() == QDialog::Accepted);
    var = QVariant(se->_value->text());
    active = se->_active->isChecked();
    delete se;
    se = 0;
  } else if(varType == NewVariant::tr("Int")) {
    ie = new IntEdit(this);
    ie->_name->setText(name);
    ok = (ie->exec() == QDialog::Accepted);
    var = QVariant(ie->_value->text().toInt());
    active = ie->_active->isChecked();
    delete ie;
    ie = 0;
  } else if(varType == NewVariant::tr("Double")) {
    de = new DoubleEdit(this);
    de->_name->setText(name);
    ok = (de->exec() == QDialog::Accepted);
    var = QVariant(de->_value->text().toDouble());
    active = de->_active->isChecked();
    delete de;
    de = 0;
  } else if(varType == NewVariant::tr("Bool")) {
    be = new BoolEdit(this);
    be->_name->setText(name);
    ok = (be->exec() == QDialog::Accepted);
    var = QVariant(be->value(), 0);
    active = be->_active->isChecked();
    delete be;
    be = 0;
  } else if(varType == NewVariant::tr("List")) {
    le = new ListEdit(this);
    le->_name->setText(name);
    ok = (le->exec() == QDialog::Accepted);
    var = QVariant(le->list());
    active = le->_active->isChecked();
    delete le;
    le = 0;
  } else {
    QMessageBox::warning(this, tr("Unknown Type"), QString(tr("I do not understand the type %1.")).arg(varType));
    return;
  }

  if(!ok)
    return;

  _params[name] = var;

  int r = _table->numRows();
  _table->setNumRows(r+1);
  QCheckTableItem * ctItem = new QCheckTableItem(_table, QString::null);
  ctItem->setChecked(active);
  _table->setItem(r, 0, ctItem);
  _table->setText(r, 1, name);
  _table->setText(r, 2, var.typeName());
  _table->setText(r, 3, var.toString());
}


void RenderWindow::sEdit()
{
  if(_table->currentSelection() == -1)
    return;

  bool do_update = false;

  int r = _table->currentRow();
  QCheckTableItem * ctItem = (QCheckTableItem*)_table->item(r, 0);
  if(ctItem == 0)
    return;
  bool active = ctItem->isChecked();
  QString name = _table->text(r, 1);
  QVariant var = _params[name];

  BoolEdit * be = 0;
  IntEdit * ie = 0;
  DoubleEdit * de = 0;
  StringEdit * se = 0;
  ListEdit * le = 0;

  switch(var.type()) {
    case QVariant::Bool:
      be = new BoolEdit(this);
      be->_name->setText(name);
      be->_active->setChecked(active);
      be->setValue(var.toBool());
      if(be->exec() == QDialog::Accepted) {
        var = QVariant(be->value(), 0);
        active = be->_active->isChecked();
        do_update = TRUE;
      }
      delete be;
      be = 0;
      break;
    case QVariant::Int:
      ie = new IntEdit(this);
      ie->_name->setText(name);
      ie->_active->setChecked(active);
      ie->_value->setText(QString::number(var.toInt()));
      if(ie->exec() == QDialog::Accepted) {
        var = QVariant(ie->_value->text().toInt());
        active = ie->_active->isChecked();
        do_update = TRUE;
      }
      delete ie;
      ie = 0;
      break;
    case QVariant::Double:
      de = new DoubleEdit(this);
      de->_name->setText(name);
      de->_active->setChecked(active);
      de->_value->setText(QString::number(var.toDouble()));
      if(de->exec() == QDialog::Accepted) {
        var = QVariant(de->_value->text().toDouble());
        active = de->_active->isChecked();
        do_update = TRUE;
      }
      delete de;
      de = 0;
      break;
    case QVariant::String:
      se = new StringEdit(this);
      se->_name->setText(name);
      se->_active->setChecked(active);
      se->_value->setText(var.toString());
      if(se->exec() == QDialog::Accepted) {
        var = QVariant(se->_value->text());
        active = se->_active->isChecked();
        do_update = TRUE;
      }
      delete se;
      se = 0;
      break;
    case QVariant::List:
      le = new ListEdit(this);
      le->_name->setText(name);
      le->_active->setChecked(active);
      le->setList(var.toList());
      if(le->exec() == QDialog::Accepted) {
        var = QVariant(le->list());
        active = le->_active->isChecked();
        do_update = TRUE;
      }
      delete le;
      le = 0;
      break;
    default:
      QMessageBox::warning(this, tr("Warning"), QString(tr("I do not know how to edit QVariant type %1.")).arg(var.typeName()));
  };
  
  if(do_update) {
    _params[name] = var;
    ctItem->setChecked(active);
    _table->setText(r, 1, name);
    _table->setText(r, 2, var.typeName());
    _table->setText(r, 3, var.toString());
  }
}


void RenderWindow::sDelete()
{
  if(_table->currentSelection() == -1)
    return;

  QString name = _table->text( _table->currentRow(), 1);
  _params.erase(name);
  _table->removeRow(_table->currentRow());
}


ParameterList RenderWindow::getParameterList()
{
  ParameterList plist;
 
  QString name;
  QVariant value;
  QCheckTableItem * ctItem = 0;
  for(int r = 0; r < _table->numRows(); r++) {
    ctItem = (QCheckTableItem*)_table->item(r, 0);
    if(ctItem->isChecked()) {
      name = _table->text(r, 1);
      value = _params[name];
      plist.append(name, value);
    }
  }
    
  return plist;
}
