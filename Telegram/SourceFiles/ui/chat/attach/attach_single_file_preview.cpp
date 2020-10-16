/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/attach/attach_single_file_preview.h"

#include "ui/chat/attach/attach_prepare.h"
#include "ui/text/format_values.h"
#include "ui/text/text_options.h"
#include "ui/image/image_prepare.h"
#include "ui/cached_round_corners.h"
#include "core/mime_type.h"
#include "styles/style_chat.h"
#include "styles/style_boxes.h"

#include <QtCore/QFileInfo>

namespace Ui {

SingleFilePreview::SingleFilePreview(
	QWidget *parent,
	const PreparedFile &file)
: RpWidget(parent) {
	preparePreview(file);

	auto h = _fileThumb.isNull()
		? (st::msgFilePadding.top() + st::msgFileSize + st::msgFilePadding.bottom())
		: (st::msgFileThumbPadding.top() + st::msgFileThumbSize + st::msgFileThumbPadding.bottom());
	resize(width(), st::boxPhotoPadding.top() + h + st::msgShadow);
}

void SingleFilePreview::prepareThumb(const QImage &preview) {
	if (preview.isNull()) {
		return;
	}

	auto originalWidth = preview.width();
	auto originalHeight = preview.height();
	auto thumbWidth = st::msgFileThumbSize;
	if (originalWidth > originalHeight) {
		thumbWidth = (originalWidth * st::msgFileThumbSize)
			/ originalHeight;
	}
	auto options = Images::Option::Smooth
		| Images::Option::RoundedSmall
		| Images::Option::RoundedTopLeft
		| Images::Option::RoundedTopRight
		| Images::Option::RoundedBottomLeft
		| Images::Option::RoundedBottomRight;
	_fileThumb = PixmapFromImage(Images::prepare(
		preview,
		thumbWidth * style::DevicePixelRatio(),
		0,
		options,
		st::msgFileThumbSize,
		st::msgFileThumbSize));
}

void SingleFilePreview::preparePreview(const PreparedFile &file) {
	auto preview = QImage();
	if (const auto image = std::get_if<PreparedFileInformation::Image>(
		&file.information->media)) {
		preview = image->data;
	} else if (const auto video = std::get_if<PreparedFileInformation::Video>(
		&file.information->media)) {
		preview = video->thumbnail;
	}
	prepareThumb(preview);
	const auto filepath = file.path;
	if (filepath.isEmpty()) {
		//auto filename = filedialogDefaultName(
		//	qsl("image"),
		//	qsl(".png"),
		//	QString(),
		//	true); // #TODO files
		auto filename = "image.png";
		_nameText.setText(
			st::semiboldTextStyle,
			filename,
			NameTextOptions());
		_statusText = u"%1x%2"_q.arg(preview.width()).arg(preview.height());
		_statusWidth = qMax(_nameText.maxWidth(), st::normalFont->width(_statusText));
		_fileIsImage = true;
	} else {
		auto fileinfo = QFileInfo(filepath);
		auto filename = fileinfo.fileName();
		_fileIsImage = Core::FileIsImage(filename, Core::MimeTypeForFile(fileinfo).name());

		auto songTitle = QString();
		auto songPerformer = QString();
		if (file.information) {
			if (const auto song = std::get_if<PreparedFileInformation::Song>(
					&file.information->media)) {
				songTitle = song->title;
				songPerformer = song->performer;
				_fileIsAudio = true;
			}
		}

		const auto nameString = ComposeNameString(
			filename,
			songTitle,
			songPerformer);
		_nameText.setText(
			st::semiboldTextStyle,
			nameString,
			NameTextOptions());
		_statusText = FormatSizeText(fileinfo.size());
		_statusWidth = qMax(
			_nameText.maxWidth(),
			st::normalFont->width(_statusText));
	}
}

void SingleFilePreview::paintEvent(QPaintEvent *e) {
	Painter p(this);

	auto w = width() - st::boxPhotoPadding.left() - st::boxPhotoPadding.right();
	auto h = _fileThumb.isNull() ? (st::msgFilePadding.top() + st::msgFileSize + st::msgFilePadding.bottom()) : (st::msgFileThumbPadding.top() + st::msgFileThumbSize + st::msgFileThumbPadding.bottom());
	auto nameleft = 0, nametop = 0, nameright = 0, statustop = 0, linktop = 0;
	if (_fileThumb.isNull()) {
		nameleft = st::msgFilePadding.left() + st::msgFileSize + st::msgFilePadding.right();
		nametop = st::msgFileNameTop;
		nameright = st::msgFilePadding.left();
		statustop = st::msgFileStatusTop;
	} else {
		nameleft = st::msgFileThumbPadding.left() + st::msgFileThumbSize + st::msgFileThumbPadding.right();
		nametop = st::msgFileThumbNameTop;
		nameright = st::msgFileThumbPadding.left();
		statustop = st::msgFileThumbStatusTop;
		linktop = st::msgFileThumbLinkTop;
	}
	auto namewidth = w - nameleft - (_fileThumb.isNull() ? st::msgFilePadding.left() : st::msgFileThumbPadding.left());
	int32 x = (width() - w) / 2, y = st::boxPhotoPadding.top();

	FillRoundRect(p, x, y, w, h, st::msgOutBg, MessageOutCorners, &st::msgOutShadow);

	if (_fileThumb.isNull()) {
		QRect inner(style::rtlrect(x + st::msgFilePadding.left(), y + st::msgFilePadding.top(), st::msgFileSize, st::msgFileSize, width()));
		p.setPen(Qt::NoPen);
		p.setBrush(st::msgFileOutBg);

		{
			PainterHighQualityEnabler hq(p);
			p.drawEllipse(inner);
		}

		auto &icon = _fileIsAudio
			? st::historyFileOutPlay
			: _fileIsImage
			? st::historyFileOutImage
			: st::historyFileOutDocument;
		icon.paintInCenter(p, inner);
	} else {
		QRect rthumb(style::rtlrect(x + st::msgFileThumbPadding.left(), y + st::msgFileThumbPadding.top(), st::msgFileThumbSize, st::msgFileThumbSize, width()));
		p.drawPixmap(rthumb.topLeft(), _fileThumb);
	}
	p.setFont(st::semiboldFont);
	p.setPen(st::historyFileNameOutFg);
	_nameText.drawLeftElided(p, x + nameleft, y + nametop, namewidth, width());

	auto &status = st::mediaOutFg;
	p.setFont(st::normalFont);
	p.setPen(status);
	p.drawTextLeft(x + nameleft, y + statustop, width(), _statusText);
}

} // namespace Ui