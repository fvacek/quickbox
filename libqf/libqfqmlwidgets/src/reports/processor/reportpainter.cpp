
//
// Author: Frantisek Vacek <fanda.vacek@gmail.com>, (C) 2014
//
// Copyright: See COPYING file that comes with this distribution
//

#include "reportpainter.h"
#include "style/pen.h"
#include "style/color.h"

#include <qf/core/log.h>
#include <qf/core/assert.h>

#include <QJsonDocument>

#include <typeinfo>

using namespace qf::qmlwidgets::reports;

//=================================================
//                              ReportItemMetaPaint
//=================================================
//const QString ReportItemMetaPaint::currentPageReportSubstitution = "@{#}";
const QString ReportItemMetaPaint::pageCountReportSubstitution = "@{n}";
//const QString ReportItemMetaPaint::checkOnReportSubstitution = "@{check:1}";
const QString ReportItemMetaPaint::checkReportSubstitution = "@{check:${STATE}}";
const QRegExp ReportItemMetaPaint::checkReportSubstitutionRegExp = QRegExp("@\\{check:(\\d)\\}");

ReportItemMetaPaint::ReportItemMetaPaint()
	: Super(NULL)
{
	//f_layoutSettings = NULL;
}

ReportItemMetaPaint::ReportItemMetaPaint(ReportItemMetaPaint *_parent, ReportItem *report_item)
	: Super(_parent)
{
	if(!report_item)
		QF_EXCEPTION("report_item is NULL.");
	if(!report_item->processor())
		QF_EXCEPTION("report_item->processor is NULL.");
	f_reportItem = report_item;
	//f_reportProcessor = report_item->processor;
	//context = report_item->processor->context();
	//f_reportItemPath = report_item->path();
	//f_layoutSettings = NULL;
    //if(report_item && report_item->processor())
    //	f_procesorContext = report_item->processor()->context();
	{
		double fill_vertical_layout_ratio = report_item->childSize(ReportItem::LayoutVertical).fillLayoutRatio();
		setFillVLayoutRatio(fill_vertical_layout_ratio);
	}
}

ReportItemMetaPaint::~ ReportItemMetaPaint()
{
	//SAFE_DELETE(f_layoutSettings);
}

ReportItemMetaPaint* ReportItemMetaPaint::child(int ix) const
{
	ReportItemMetaPaint *ret = dynamic_cast<ReportItemMetaPaint*>(Super::child(ix));
	if(!ret) {
		qfWarning() << "Child at index" << ix << "is not a kind of ReportItemMetaPaint.";
		qfInfo() << qf::core::Log::stackTrace();
	}
	return ret;
}

ReportItem* ReportItemMetaPaint::reportItem()
{
	return f_reportItem;
}

void ReportItemMetaPaint::paint(ReportPainter *painter, unsigned mode)
{
	foreach(Super *_it, children()) {
		ReportItemMetaPaint *it = static_cast<ReportItemMetaPaint*>(_it);
		it->paint(painter, mode);
	}
}

void ReportItemMetaPaint::setInset(qreal hinset, qreal vinset)
{
	if(insetHorizontal() == hinset && insetVertical() == vinset) {
	}
	else {
		f_layoutSettings[LayoutSetting::HInset] = hinset;
		f_layoutSettings[LayoutSetting::VInset] = vinset;
	}
}

void ReportItemMetaPaint::shiftChildren(const ReportItem::Point offset)
{
	foreach(Super *_it, children()) {
		ReportItemMetaPaint *it = static_cast<ReportItemMetaPaint*>(_it);
		it->renderedRect.translate(offset);
		it->shiftChildren(offset);
	}
}

void ReportItemMetaPaint::expandChildrenFramesRecursively()
{
	if(renderedRect.flags & ReportItem::Rect::LayoutHorizontalFlag) {
		/// ve smeru layoutu natahni jen posledni dite az po inset
		ReportItemMetaPaint *it = lastChild();
		if(it && it->isExpandable()) {
			it->renderedRect.setRight(renderedRect.right() - insetHorizontal());
		}
	}
	else {
		/// ve smeru layoutu natahni jen posledni dite az po inset
		ReportItemMetaPaint *it = lastChild();
		if(it && it->isExpandable()) {
			it->renderedRect.setBottom(renderedRect.bottom() - insetVertical());
		}
	}
	foreach(Super *_it, children()) {
		/// ve smeru ortogonalnim k layoutu natahni vsechny deti
		ReportItemMetaPaint *it = static_cast<ReportItemMetaPaint*>(_it);
		if(it->isExpandable()) {
			if(renderedRect.flags & ReportItem::Rect::LayoutHorizontalFlag) {
				it->renderedRect.setTop(renderedRect.top() + insetVertical());
				it->renderedRect.setBottom(renderedRect.bottom() - insetVertical());
			}
			else {
				it->renderedRect.setLeft(renderedRect.left() + insetHorizontal());
				it->renderedRect.setRight(renderedRect.right() - insetHorizontal());
			}
		}
	}
}

bool ReportItemMetaPaint::hasSpringChildrenFramesInVerticalLayout()
{
	bool has_expandable_children = false;
	for(int i=0; i<childrenCount(); i++) {
		ReportItemMetaPaint *it = child(i);
		double d = it->fillVLayoutRatio();
		if(d >= 0) {
			has_expandable_children = true;
			break;
		}
	}
	return has_expandable_children;
}

style::CompiledTextStyle ReportItemMetaPaint::effectiveTextStyle()
{
	style::CompiledTextStyle ret;
	ReportItemMetaPaint *it = this;
	while(it) {
		ret = it->textStyle();
		if(!ret.isNull())
			break;
		it = it->parent();
	}
	QF_ASSERT(!ret.isNull(), "Cannot find TextStyle definition in parents", return ret);
	return ret;
}

bool ReportItemMetaPaint::expandChildVerticalSpringFrames()
{
	qfLogFuncFrame() << "rendered rect:" << renderedRect.toString();
	bool has_expandable_children = false;
	for(int i=0; i<childrenCount(); i++) {
		ReportItemMetaPaint *it = child(i);
		double d = it->fillVLayoutRatio();
		if(d >= 0) {
			has_expandable_children = true;
			break;
		}
	}
	if(!has_expandable_children) return has_expandable_children;

	//static int reccnt = 0;
	//reccnt++;
	//if(reccnt == 3) return has_expandable_children;

	double layout_size = renderedRect.height();
	double layout_inset = insetVertical();
	layout_size -= 2 * layout_inset;
	/// rozpocitej procenta, je to napsano pro oba layouty, ale asi to nepujde kvuli zalamovani textu pouzit nikdy pro horizontalni layout
	if(layout() == qf::qmlwidgets::graphics::LayoutVertical) {
		qreal sum_percent = 0;
		int cnt_0_percent = 0;
		qreal sum_mm = 0;
		QList<int> spring_children_ixs;
		//bool has_expandable_children = false;
		for(int i=0; i<childrenCount(); i++) {
			ReportItemMetaPaint *it = child(i);
			double d = it->fillVLayoutRatio();
			if(d < 0) {
				sum_mm += it->renderedRect.height();
			}
			else {
				if(d == 0) cnt_0_percent++;
				else sum_percent += d;
				spring_children_ixs << i;
			}
		}
		if(spring_children_ixs.count()) {
			qreal rest_percent = 1 - sum_percent;
			if(rest_percent < 0) rest_percent = 0;
			qreal percent_0 = 0;
			if(cnt_0_percent > 0) percent_0 = rest_percent / cnt_0_percent;

			double rest_mm = layout_size - sum_mm;
			//reklamacewqfInfo() << "layout_size:" << layout_size << "sum_mm:" << sum_mm;
			if(rest_mm < 0) rest_mm = 0;
			double children_ly_offset = insetVertical();
			for(int i=0; i<childrenCount(); i++) {
				ReportItemMetaPaint *it = child(i);
				if(children_ly_offset > 0) {
					/// nejdriv posun deti, nafouknuti itemu
					ReportItem::Point p;
					p.setY(children_ly_offset);
					it->shift(p);
				}
				if(spring_children_ixs.contains(i)) {
					double curr_ly_size = it->renderedRect.height();
					double new_ly_size = it->fillVLayoutRatio();
					if(new_ly_size == 0) new_ly_size = percent_0;
					new_ly_size = new_ly_size * rest_mm;
					//qfInfo() << reccnt << "expanding:" << it->reportItem()->element.tagName() << " id:" << it->reportItem()->element.attribute("id") << "#" << i << "rest mm:" << rest_mm << "new ly_size:" << new_ly_size;
					double ly_size_offset = new_ly_size - curr_ly_size;
					if(ly_size_offset < 0) {
						//qfWarning() << "new_ly_size:" << new_ly_size << "is smaller than old one:" << curr_ly_size << "ignoring";
					}
					else {
						it->renderedRect.setHeight(new_ly_size);
						it->alignChildren(); /// kdyz neco expanduju, tak to musim taky zarovnat
						it->expandChildVerticalSpringFrames();
						children_ly_offset += ly_size_offset;
					}
				}
			}
		}
	}
	/*
	else if(layout() == qf::qmlwidgets::graphics::LayoutHorizontal) {
		for(int i=0; i<childrenCount(); i++) {
			ReportItemMetaPaint *it = child(i);
			double d = it->fillVLayoutRatio();
			if(d >= 0) {
				if(d == 0) d = 1;
				double new_ly_size = layout_size * d;
				//qfInfo() << layout_size << d << new_ly_size;
				it->renderedRect.setHeight(new_ly_size);
				//qfInfo() << renderedRect.toString();
			}
		}
	}
	*/
	qfDebug() << "\t expanded rendered rect:" << renderedRect.toString();
	//reccnt--;
	return has_expandable_children;
}

void ReportItemMetaPaint::alignChildren()
{
	qfLogFuncFrame();
	Rect dirty_rect = renderedRect.adjusted(insetHorizontal(), insetVertical(), -insetHorizontal(), -insetVertical());
	//qfDebug() << "\t TAG:" << reportItem()->element.tagName();
	qfDebug() << "\t dirty_rect:" << dirty_rect.toString();
	qfDebug() << "\t alignment:" << alignment();
	qfDebug() << "\t layout:" << layout();
	if(dirty_rect.isValid()) {
		if(alignment() & ~(Qt::AlignLeft | Qt::AlignTop)) {
			Point offset;
			if(layout() == qf::qmlwidgets::graphics::LayoutVertical && hasSpringChildrenFramesInVerticalLayout()) {
				/// pokud ma spring item ve vertikalnim layoutu, neni treba nic posouvat, o to se postara expandChildVerticalSpringFrames()
			}
			else {
				/// ve smeru layoutu posun cely blok
				Rect r1;
				/// vypocitej velikost potisknuteho bloku
				for(int i=0; i<childrenCount(); i++) {
					ReportItemMetaPaint *it = child(i);
					qfDebug() << "\t\t item potisknuty blok:" << it->renderedRect.toString();
					if(i == 0) r1 = it->renderedRect;
					else r1 = r1.united(it->renderedRect);
				}
				qfDebug() << "\t potisknuty blok:" << r1.toString();
				qreal al = 0, d;
				if(layout() == qf::qmlwidgets::graphics::LayoutHorizontal) {
					if(alignment() & Qt::AlignHCenter) al = 0.5;
					else if(alignment() & Qt::AlignRight) al = 1;
					d = dirty_rect.width() - r1.width();
					if(al > 0 && d > 0)  {
						offset.rx() = d * al - (r1.left() - dirty_rect.left());
					}
				}
				else if(layout() == qf::qmlwidgets::graphics::LayoutVertical) {
					if(alignment() & Qt::AlignVCenter) al = 0.5;
					else if(alignment() & Qt::AlignBottom) al = 1;
					d = dirty_rect.height() - r1.height();
					if(al > 0 && d > 0)  {
						offset.ry() = d * al - (r1.top() - dirty_rect.top());
					}
				}
			}
			qfDebug() << "\t offset ve smeru layoutu:" << offset.toString();
			/// v orthogonalnim smeru kazdy item
			for(int i=0; i<childrenCount(); i++) {
				ReportItemMetaPaint *it = child(i);
				const Rect &r1 = it->renderedRect;
				qfDebug() << "\t\titem renderedRect:" << r1.toString();
				qreal al = 0, d;

				if(orthogonalLayout() == qf::qmlwidgets::graphics::LayoutHorizontal) {
					offset.rx() = 0;
					if(alignment() & Qt::AlignHCenter) al = 0.5;
					else if(alignment() & Qt::AlignRight) al = 1;
					d = dirty_rect.width() - r1.width();
					if(al > 0 && d > 0)  {
						qfDebug() << "\t\thorizontal alignment:" << al;
						offset.rx() = d * al - (r1.left() - dirty_rect.left());
					}
				}
				else if(orthogonalLayout() == qf::qmlwidgets::graphics::LayoutVertical) {
					offset.ry() = 0;
					al = 0;
					if(alignment() & Qt::AlignVCenter) al = 0.5;
					else if(alignment() & Qt::AlignBottom) al = 1;
					d = dirty_rect.height() - r1.height();
					if(al > 0 && d > 0)  {
						qfDebug() << "\t\tvertical alignment:" << al;
						offset.ry() = d * al - (r1.top() - dirty_rect.top());
					}
				}
				qfDebug() << "\t\talign offset:" << offset.toString();
				if(!offset.isNull()) it->shift(offset);
			}
		}
	}
}

QString ReportItemMetaPaint::dump(int indent)
{
	QString indent_str;
	indent_str.fill(' ', indent);
	const char *type_name = typeid(*this).name();
	QString ret = QString("%1[%2] 0x%3").arg(indent_str).arg(type_name).arg((qulonglong)this, 0, 16);
	//QString ret = QString("%1[%2] 0x%3").arg(indent_str).arg(type_name).arg((qulonglong)this, 0, 16);
	ReportItemMetaPaintFrame *frm = dynamic_cast<ReportItemMetaPaintFrame*>(this);
	if(frm) ret += " : " + frm->renderedRect.toString();
	ret += "\n";
	foreach(Super *_it, children()) {
		ReportItemMetaPaint *it = static_cast<ReportItemMetaPaint*>(_it);
		ret += it->dump(indent + 2);
	}
	return ret;
}

//=================================================
//                              ReportItemMetaPaintReport
//=================================================
ReportItemMetaPaintReport::ReportItemMetaPaintReport(ReportItem *report_item)
	: ReportItemMetaPaint(NULL, report_item)
{
	//f_reportProcessor = report_item->processor;
	/*--
	QString s = report_item->property("orientation", "portrait");
	if(s == "landscape") orientation = QPrinter::Landscape;
	else orientation = QPrinter::Portrait;
	pageSize = QSize(report_item->property("w").toInt(), report_item->property("h").toInt());
	--*/
}

//=================================================
//           ReportItemMetaPaintFrame
//=================================================
ReportItemMetaPaintFrame::ReportItemMetaPaintFrame(ReportItemMetaPaint *_parent, ReportItem *report_item)
: ReportItemMetaPaint(_parent, report_item), lbrd(Qt::NoPen), rbrd(Qt::NoPen), tbrd(Qt::NoPen), bbrd(Qt::NoPen)
{
	//qfDebug() << QF_FUNC_NAME << reportElement.tagName();
    QF_ASSERT_EX(report_item != nullptr, "ReportItem is NULL");
    ReportItemFrame *frame_item = qobject_cast<ReportItemFrame*>(report_item);
    if(frame_item) {
        {
            style::Color *c = frame_item->fill();
            if(c)
                fill = c->color();
        }
        {
            style::Pen *p = frame_item->border();
            if(p)
                lbrd = rbrd = tbrd = bbrd = p->pen();
        }
    }
    /*--
	QString s = report_item->property("fill").toString();
	if(!s.isEmpty()) {
		if(s.startsWith("{grid:")) {
			s.replace('|', '"');
			QJsonDocument json_doc = QJsonDocument::fromJson(s.toUtf8());
			alternativeFillDef = json_doc.toVariant();
		}
		else
			fill = context().styleCache().brush(s);
	}
	s = report_item->property("lbrd").toString();
	if(!s.isEmpty())
		lbrd = context().styleCache().pen(s);
	s = report_item->property("rbrd").toString();
	if(!s.isEmpty())
		rbrd = context().styleCache().pen(s);
	s = report_item->property("tbrd").toString();
	if(!s.isEmpty())
		tbrd = context().styleCache().pen(s);
	s = report_item->property("bbrd").toString();
	if(!s.isEmpty())
		bbrd = context().styleCache().pen(s);
    --*/
    //qfDebug() << "\tRETURN";
}

void ReportItemMetaPaintFrame::paint(ReportPainter *painter, unsigned mode)
{
	//qfDebug() << QF_FUNC_NAME << reportElement.tagName();
	QF_ASSERT(painter, "painter is NULL", return);
	//qfDebug() << "\trenderedRect:" << renderedRect.toString();
	bool selected = (painter->selectedItem() && painter->selectedItem() == this);
	if(mode & PaintFill)
		fillItem(painter, selected);
	ReportItemMetaPaint::paint(painter, mode);
	//if(selected) qfDebug() << "\tBINGO";
	if(mode & PaintBorder)
		frameItem(painter, selected);
}

void ReportItemMetaPaintFrame::fillItem(QPainter *painter, bool selected)
{
	Rect r = qf::qmlwidgets::graphics::mm2device(renderedRect, painter->device());
	//qfDebug().color(QFLog::Yellow) << QF_FUNC_NAME << reportElement.tagName();
	//qfInfo() << "\t logicalDpiX:" << painter->device()->logicalDpiX();
	//qfInfo() << "\t logicalDpiY:" << painter->device()->logicalDpiY();
	//qfInfo() << "\t rendered rect:" << renderedRect.toString();
	//qfInfo() << "\t br:" << r.toString();
	//qfDebug() << "\tbrush color:"
	if(selected) {
		painter->fillRect(r, QColor("#FFEEEE"));
	}
	else {
		QVariantMap alterm = alternativeFillDef.toMap();
		if(alterm.isEmpty()) {
			if(fill.style() != Qt::NoBrush)
				painter->fillRect(r, fill);
		}
        /*--
		else {
			if(alterm.contains("grid")) {
				alterm = alterm.value("grid").toMap();
				double w = qf::qmlwidgets::graphics::x2device(alterm.value("w").toDouble(), painter->device());
				double h = qf::qmlwidgets::graphics::y2device(alterm.value("h").toDouble(), painter->device());
				QPen p = context().styleCache().pen(alterm.value("pen").toString());
				painter->setPen(p);
				for(double x=r.left(); x<r.right(); x+=w) {
					painter->drawLine(x, r.top(), x, r.bottom());
				}
				for(double y=r.top(); y<r.bottom(); y+=h) {
					painter->drawLine(r.left(), y, r.right(), y);
				}
			}
		}
        --*/
		//QFString s = element.attribute("fill");
		//if(!!s) painter->fillRect(r, context().brushFromString(s));
	}
	//painter->fillRect(r, QColor("orange"));
}

void ReportItemMetaPaintFrame::frameItem(QPainter *painter, bool selected)
{
    Q_UNUSED(selected);
    /*--
	QString s;
	if(selected) {
		s = "color: magenta; style: solid; size:2";
		painter->setPen(context().styleCache().pen(s));
		painter->setBrush(QBrush());
		painter->drawRect(qf::qmlwidgets::graphics::mm2device(renderedRect, painter->device()));
	}
	else {
		drawLine(painter, LBrd, lbrd);
		drawLine(painter, TBrd, tbrd);
		drawLine(painter, RBrd, rbrd);
		drawLine(painter, BBrd, bbrd);
	}
    --*/
    drawLine(painter, LBrd, lbrd);
    drawLine(painter, TBrd, tbrd);
    drawLine(painter, RBrd, rbrd);
    drawLine(painter, BBrd, bbrd);
}

void ReportItemMetaPaintFrame::drawLine(QPainter *painter, LinePos where, const QPen &_pen)
{
	if(_pen.widthF() == 0) return;
	QPen pen = _pen;
	/// preved tiskarske body na body vystupniho zarizeni
	qreal w = pen.widthF() * 25.4 / 72;
	/// ted je w v milimetrech
	bool horizontal = (where == TBrd || where == BBrd);
	if(horizontal) pen.setWidthF(qf::qmlwidgets::graphics::y2device(w, painter->device()));
	else pen.setWidthF(qf::qmlwidgets::graphics::x2device(w, painter->device()));
	Point p1, p2;
	Rect r = qf::qmlwidgets::graphics::mm2device(renderedRect, painter->device());
	if(where == TBrd) { p1 = r.topLeft(); p2 = r.topRight(); }
	else if(where == LBrd) { p1 = r.topLeft(); p2 = r.bottomLeft(); }
	else if(where == BBrd) { p1 = r.bottomLeft(); p2 = r.bottomRight(); }
	else if(where == RBrd) { p1 = r.topRight(); p2 = r.bottomRight(); }
	if(!(p1 == p2)) {
		painter->setPen(pen);
		painter->drawLine(p1, p2);
	}
}
//=================================================
//                              ReportItemMetaPaintPage
//=================================================
/*
ReportItemMetaPaintPage::ReportItemMetaPaintPage(ReportItemMetaPaint *parent, const QFDomElement &el, const ReportProcessor::Context &context)
	: ReportItemMetaPaintFrame(parent, el, context)
{
}

void ReportItemMetaPaintPage::paint(ReportPainter *painter)
{
	qfDebug() << QF_FUNC_NAME << reportElement.tagName();
	qfDebug() << "\trenderedRect:" << renderedRect.toString();
	//qfDebug() << "\tchildren cnt:" << children.count();
	//painter->fillRect(renderedRect, context().brushFromString("color: white"));
	ReportItemMetaPaintFrame::paint(painter);
}
	*/

//=================================================
//                              ReportItemMetaPaintText
//=================================================
void ReportItemMetaPaintText::paint(ReportPainter *painter, unsigned mode)
{
	//qfDebug() << QF_FUNC_NAME << reportElement.tagName();
	QF_ASSERT(painter, "painter is NULL", return);
	if(mode != PaintFill)
		return;

	//bool is_yellow = false;
	ReportPainter *rep_painter = dynamic_cast<ReportPainter*>(painter);
	if(rep_painter && rep_painter->isMarkEditableSqlText() && !sqlId.isEmpty()) {
		/// zazlut cely parent frame, az do ktereho se muze editovatelny text roztahnout
		ReportItemMetaPaintFrame *it = dynamic_cast<ReportItemMetaPaintFrame*>(parent());
		if(it) {
			Rect r = qf::qmlwidgets::graphics::mm2device(it->renderedRect, painter->device());
			painter->fillRect(r, Qt::yellow);
			//is_yellow = true;
		}
	}

	QFontMetricsF font_metrics = QFontMetricsF(painter->font(), painter->device());
	//ReportItemMetaPaintFrame::paint(painter);
	//qfDebug() << "\trenderedRect:" << renderedRect.toString();
	//painter->setBrush(brush);
	painter->setPen(pen);
	painter->setFont(font);
	QString s = text;
	//if(is_yellow) qfInfo() << s.toUtf8().toHex() << s.length();// << s.replace("\n", "<LF>").replace("\r", "<CR>");
	if(text.indexOf('@') >= 0) {
		//s = s.replace(currentPageReportSubstitution, QString::number(painter->currentPage + 1));
		s = s.replace(pageCountReportSubstitution, QString::number(painter->pageCount));
	}
	Rect br = qf::qmlwidgets::graphics::mm2device(renderedRect, painter->device());
	br.adjust(0, 0, 1, 1); /// nekdy se stane, kvuji nepresnostem prepocitavani jednotek, ze se to vyrendruje pri tisku jinak, nez pri kompilaci, tohle trochu pomaha:)
	//r.setHeight(500);
	//qfWarning().noSpace() << "'" << s << "' flags: " << flags;
#if 0
	painter->drawText(br, flags, s);
#else
	/// to samy jako v #if 0, jen se to tiskne stejnym zpusobem, jako se to kompilovalo, coz muze ukazat, proc to vypada jinak, nez cekam
	qreal leading = font_metrics.leading();
	qreal height = 0;
	//qreal width = 0;
	QTextLayout textLayout;
	//Qt::Alignment alignment = (~Qt::Alignment()) & flags;
	QTextOption opt = textOption;
	opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	textLayout.setTextOption(opt);
	textLayout.setFont(painter->font());
	textLayout.setText(s);
	textLayout.beginLayout();
	while (1) {
		QTextLine line = textLayout.createLine();
		if(!line.isValid()) {
			break;
		}
		line.setLineWidth(br.width());
		if(height > 0) height += leading;
		line.setPosition(QPointF(0., height));
		height += line.height();
		//width = qMax(width, line.naturalTextWidth());
	}
	textLayout.endLayout();
	textLayout.draw(painter, br.topLeft());
#endif
}

QString ReportItemMetaPaintText::dump(int indent)
{
	QString indent_str;
	indent_str.fill(' ', indent);
	QString ret = QString("%1[%2] 0x%3").arg(indent_str).arg(typeid(*this).name()).arg((qulonglong)this, 0, 16);
	//QString ret = QString("%1[%2] 0x%3").arg(indent_str).arg(typeid(*this).name()).arg((qulonglong)this, 0, 16);
	ret += QString(" '%1'\n").arg(text);
	foreach(auto _it, children()) {
		ReportItemMetaPaint *it = static_cast<ReportItemMetaPaint*>(_it);
		ret += it->dump(indent + 2);
	}
	return ret;
}

//=================================================
//           ReportItemMetaPaintCheck
//=================================================
void ReportItemMetaPaintCheck::paint(ReportPainter * painter, unsigned mode)
{
	//qfDebug() << QF_FUNC_NAME << reportElement.tagName();
	QF_ASSERT(painter, "painter is NULL", return);
	if(mode != PaintFill)
		return;

	QFontMetricsF font_metrics = QFontMetricsF(painter->font(), painter->device());

	//qfInfo() << "paint" << text;
	QRegExp rx = ReportItemMetaPaint::checkReportSubstitutionRegExp;
	if(rx.exactMatch(text)) {
		bool check_on = rx.capturedTexts().value(1) == "1";
		/// V tabulkach by jako check OFF slo netisknout vubec nic,
		/// ale na ostatnich mistech repotu je to zavadejici .

		//qfInfo() << check_on;
		/// BOX
        //QString s_box;
        //s_box = (check_on)? "color: black; style: solid; size:1": "color: gray; style: solid; size:1";
        painter->setPen(QPen(Qt::SolidLine));
		painter->setBrush(QBrush());
		ReportItem::Rect r = renderedRect;
		r.translate(0, -font_metrics.leading());
		qreal w = renderedRect.width();
		Qt::Alignment alignment_flags = textOption.alignment();
		if(alignment_flags & Qt::AlignHCenter) r.translate((w - r.width())/2., 0);
		else if(alignment_flags & Qt::AlignRight) r.translate(w - r.width(), 0);
			//r.setWidth(2* r.width() / 3.);
			//r.setHeight(2 * r.height() / 3.);
		painter->drawRect(qf::qmlwidgets::graphics::mm2device(r, painter->device()));

		if(check_on) {
#if 0
			/// CHECK hook
			static QString s_check = "color: teal; style: solid; size:2";
			painter->setPen(context().styleCache().pen(s_check));
			r = qf::qmlwidgets::graphics::mm2device(r, painter->device());
			QPointF p1(r.left(), r.top() + r.height() / 2);
			QPointF p2(r.left() + r.width() / 2, r.bottom());
			painter->drawLine(p1, p2);
			p1 = QPointF(r.right() + 0.2 * r.width(), r.top());
			painter->drawLine(p2, p1);
#else
			/// CHECK cross
            //--static QString s_check = "color: maroon; style: solid; size:2";
            QPen p(Qt::SolidLine);
            QColor c;
            c.setNamedColor("maroon");
            p.setColor(c);
            painter->setPen(p);
			r = qf::qmlwidgets::graphics::mm2device(r, painter->device());
			//QPointF p1(r.left(), r.top() + r.height() / 2);
			//QPointF p2(r.left() + r.width() / 2, r.bottom());
			painter->drawLine(r.topLeft(), r.bottomRight());
			//p1 = QPointF(r.right() + 0.2 * r.width(), r.top());
			painter->drawLine(r.bottomLeft(), r.topRight());
#endif
		}
		return;
	}
}

//=================================================
//                              ReportItemMetaPaintImage
//=================================================
void ReportItemMetaPaintImage::paint(ReportPainter *painter, unsigned mode)
{
	//qfDebug().color(QFLog::Green) << QF_FUNC_NAME << reportElement.tagName() << "mode:" << mode;
	QF_ASSERT(painter, "painter is NULL", return);
	QPrinter *printer = dynamic_cast<QPrinter*>(painter->device());
	//if(printer) { qfInfo() << "printer output format:" << printer->outputFormat() << "is native printer:" << (printer->outputFormat() == QPrinter::NativeFormat); }
	if(printer && printer->outputFormat() == QPrinter::NativeFormat) {
		if(isSuppressPrintOut()) {
			qfInfo() << "print out suppressed";
			return;
		}
	}
	if(mode != PaintFill) return;

	Rect br = qf::qmlwidgets::graphics::mm2device(renderedRect, painter->device());
	//br.adjust(0, 0, 1, 1); /// nekdy se stane, kvuji nepresnostem prepocitavani jednotek, ze se to vyrendruje pri tisku jinak, nez pri kompilaci, tohle trochu pomaha:)
	//r.setHeight(500);
	//painter->fillRect(r, QColor("#DDDDDD"));
	//qfWarning().noSpace() << "'" << s << "' flags: " << flags;
	if(image.isImage()) {
		//qfInfo() << "\t is image";
		painter->save();
		QSize sz = image.image.size();
		painter->translate(br.topLeft());
		painter->scale(br.width() / sz.width(), br.height() / sz.height());
		painter->drawImage(QPoint(0, 0), image.image);
		painter->restore();
		//painter->drawImage(br.topLeft(), image.image.scaled(br.size().toSize(), aspectRatioMode, Qt::SmoothTransformation));
	}
	else if(image.isPicture()) {
		painter->save();
		QRect pict_r = image.picture.boundingRect();
		QSize pict_sz = pict_r.size();
		painter->translate(br.topLeft());
		{
			/// dosud nepodporuji aspect ratio, zda se
			//painter->fillRect(image.picture.boundingRect(), Qt::cyan);
			//painter->scale(renderedRect.width() / sz.width(), renderedRect.height() / sz.height());
			/// pokud se rozmery a posunuti obrazku lisi od velikosti regionu pro nej urceneho, zmensi/zvetsi a posun ho tak,
			/// aby vyplnil cely region.
			painter->scale(br.width() / pict_sz.width(), br.height() / pict_sz.height());
			/// pokud pict_r nezacina na [0,0], je treba posunout painter na zacatek obrazku
			painter->translate(-pict_r.topLeft());
			//qfInfo() << "\t picture scale:" << br.width() / sz.width() << br.height() / sz.height();
			//qfInfo() << "\t rendered rect mm:" << renderedRect.toString();
			//qfInfo() << "\t rendered rect px:" << br.toString();
			//qfInfo() << "\t picture bounding rect:" << Rect(image.picture.boundingRect()).toString();
		}
		painter->drawPicture(QPoint(0, 0), image.picture);
		painter->restore();
	}
	//if(image.isImage()) painter->drawImage(br.topLeft(), image.image.scaled(br.size().toSize(), Qt::KeepAspectRatioByExpanding));
}

QString ReportItemMetaPaintImage::dump(int indent)
{
	QString indent_str;
	indent_str.fill(' ', indent);
	QString ret = QString("%1[%2] 0x%3").arg(indent_str).arg(typeid(*this).name()).arg((qulonglong)this, 0, 16);
	//QString ret = QString("%1[%2] 0x%3").arg(indent_str).arg(typeid(*this).name()).arg((qulonglong)this, 0, 16);
	ret += QString(" '%1'\n").arg("image");
	return ret;
}

//=================================================
//                              ReportPainter
//=================================================
ReportPainter::ReportPainter(QPaintDevice *device)
	: QPainter(device)
{
	//currentPage = 0;
	pageCount = 0;
	f_selectedItem = NULL;
	setMarkEditableSqlText(false);
}

void ReportPainter::drawMetaPaint(ReportItemMetaPaint *item)
{
	if(item) {
		item->paint(this, ReportItemMetaPaint::PaintFill);
		item->paint(this, ReportItemMetaPaint::PaintBorder);
	}
}




