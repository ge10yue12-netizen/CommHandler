#include "SerLhgkProtocol.h"

SerLhgkProtocol::SerLhgkProtocol(uint8_t version_major, uint8_t version_minor)
	: version_major_(version_major), version_minor_(version_minor) {
}

std::vector<uint8_t> SerLhgkProtocol::buildProtocolNegotiationFrame(bool isRequest) {
	std::vector<uint8_t> payload = {
		static_cast<uint8_t>(isRequest ? 0x01 : 0x02),
		version_major_, version_minor_
	};
	return buildFrame(0x08, payload);
}

std::vector<uint8_t> SerLhgkProtocol::buildControlCommandFrame(uint8_t cmd) {
	return buildFrame(0x02, { cmd });
}

std::vector<uint8_t> SerLhgkProtocol::buildDataChannelControlFrame(uint8_t ctrl) {
	return buildFrame(0x07, { ctrl });
}

std::vector<uint8_t> SerLhgkProtocol::buildModuleDataFrame(const  std::vector < uint8_t>& moduleId, const  std::vector < uint8_t>& dataTypeId, const  std::vector<double>& data)
{
	std::vector<uint8_t> payload;
	for (size_t i = 0; i < moduleId.size() && i < dataTypeId.size() && i < data.size(); i++)
	{
		payload.push_back(moduleId.at(i));
		payload.push_back(dataTypeId.at(i));
		appendDoubleLE(payload, data.at(i));  // v1.1使用小端序
	}

	return buildFrame(0x01, payload);
}

std::vector<uint8_t> SerLhgkProtocol::buildPollingRequestFrame() {
	return buildFrame(0x05, {});
}

std::vector<uint8_t> SerLhgkProtocol::buildExternalDataFrame(double force, double temp) {
	std::vector<uint8_t> payload;

	// 力值模块段
	payload.push_back(0x06);      // 模块编号
	payload.push_back(0x0C);      // 数据类型编号（力值，v1.1改为0x0C）
	appendDoubleLE(payload, force);

	// 温度模块段
	payload.push_back(0x06);      // 模块编号
	payload.push_back(0x0D);      // 数据类型编号（温度，v1.1改为0x0D）
	appendDoubleLE(payload, temp);

	return buildFrame(0x01, payload);
}

std::vector<uint8_t> SerLhgkProtocol::buildStatusResponseFrame(uint8_t statusCode, const std::string& message) {
	std::vector<uint8_t> payload = { statusCode };
	payload.insert(payload.end(), message.begin(), message.end());
	return buildFrame(0x03, payload);
}

std::vector<uint8_t> SerLhgkProtocol::buildErrorFrame(uint8_t errCode, const std::string& errorMsg) {
	std::vector<uint8_t> payload = { errCode };
	payload.insert(payload.end(), errorMsg.begin(), errorMsg.end());
	return buildFrame(0x04, payload);  // v1.1错误帧类型改为0x04
}

std::vector<uint8_t> SerLhgkProtocol::buildExternalDataRateFrame(uint16_t fps) {
	std::vector<uint8_t> payload = { 0x06 }; // module 0x06
	appendLittleEndian(payload, fps);  // v1.1使用小端序
	return buildFrame(0x09, payload);
}

uint8_t SerLhgkProtocol::getFrameType(const std::vector<uint8_t>& frame) {
	if (frame.size() < 3 || frame[0] != 0xAA || frame[1] != 0x55) return 0xFF;
	return frame[2];
}

void SerLhgkProtocol::getExternalData(const uint8_t& segmentCount, const std::vector<uint8_t>& payload, double& dForce, double& dTemp)
{
	dForce = 0;
	dTemp = 0;
	size_t index = 0;

	for (uint8_t i = 0; i < segmentCount; ++i) {
		if (index + 2 + 8 > payload.size()) return;

		uint8_t moduleId = payload[index++];
		uint8_t typeId = payload[index++];

		// v1.1使用小端序读取double
		uint64_t raw = 0;
		for (int j = 0; j < 8; ++j)
			raw |= ((uint64_t)payload[index++]) << (j * 8);

		double val;
		std::memcpy(&val, &raw, sizeof(val));

		if (moduleId == 0x06) {
			if (typeId == 0x0C) dForce = val;  // v1.1力值为0x0C
			else if (typeId == 0x0D) dTemp = val;  // v1.1温度为0x0D
		}
	}
}

// 探测完整帧字节数（不含 CRC 校验）；断帧返回 0，非法返回 -1
int SerLhgkProtocol::probeCompleteFrameSize(const std::vector<uint8_t>& buf) const
{
	// 帧头不足：断帧，继续等待
	if (buf.size() < 5)
		return 0;
	if (buf[0] != 0xAA || buf[1] != 0x55)
		return -1;
	// v1.1：长度字段为负载字节数（小端）
	const uint16_t length = static_cast<uint16_t>(buf[3] | (buf[4] << 8));
	// 防护：异常长度视为非法同步点，避免无限缓冲
	if (length > 4096)
		return -1;
	const int total = 9 + static_cast<int>(length);
	if (static_cast<int>(buf.size()) < total)
		return 0;
	return total;
}

bool SerLhgkProtocol::checkAndParseFrame(const std::vector<uint8_t>& buf, uint8_t& type, std::vector<uint8_t>& payload)
{
	if (buf.size() < 9) return false;

	if (buf[0] != 0xAA || buf[1] != 0x55) return false;
	if (!(buf[buf.size() - 2] == 0x0D && buf[buf.size() - 1] == 0x0A)) return false;

	type = buf[2];
	// v1.1使用小端序读取长度
	uint16_t length = buf[3] | (buf[4] << 8);

	size_t payload_start = 5;
	size_t payload_end = buf.size() - 4;  // 去掉 CRC 和帧尾

	size_t expected_payload_len = static_cast<size_t>(length);

	if (payload_end - payload_start != expected_payload_len)
		return false;

	// v1.1 CRC校验（小端序）
	uint16_t expected_crc = buf[buf.size() - 4] | (buf[buf.size() - 3] << 8);
	uint16_t actual_crc = crc16_xmodem(buf.data(), buf.size() - 4);
	if (expected_crc != actual_crc)
		return false;

	// 提取 payload
	payload.assign(buf.begin() + payload_start, buf.begin() + payload_end);
	return true;
}

std::vector<uint8_t> SerLhgkProtocol::buildFrame(uint8_t type, const std::vector<uint8_t>& payload) {
	std::vector<uint8_t> buf = { 0xAA, 0x55, type };
	appendLittleEndian(buf, payload.size());  // v1.1使用小端序
	buf.insert(buf.end(), payload.begin(), payload.end());
	uint16_t crc = crc16_xmodem(buf.data(), buf.size());
	buf.push_back(crc & 0xFF);        // v1.1 CRC使用小端序
	buf.push_back((crc >> 8) & 0xFF);
	buf.push_back(0x0D);
	buf.push_back(0x0A);
	return buf;
}

std::vector<uint8_t> SerLhgkProtocol::buildExternalFrame(uint8_t type, const std::vector<uint8_t>& payload)
{
	// v1.1不再使用此函数，统一使用buildFrame
	return buildFrame(type, payload);
}

void SerLhgkProtocol::appendLittleEndian(std::vector<uint8_t>& buf, uint16_t val) {
	buf.push_back(val & 0xFF);         // 低字节在前
	buf.push_back((val >> 8) & 0xFF);  // 高字节在后
}

void SerLhgkProtocol::appendDoubleLE(std::vector<uint8_t>& buf, double d)
{
	uint64_t raw;
	std::memcpy(&raw, &d, sizeof(d));
	// 写入为小端序（低位在前）
	for (int i = 0; i < 8; ++i) {
		buf.push_back(static_cast<uint8_t>((raw >> (i * 8)) & 0xFF));
	}
}

uint16_t SerLhgkProtocol::crc16_xmodem(const uint8_t* data, size_t length) {
	uint16_t crc = 0;
	for (size_t i = 0; i < length; ++i) {
		crc ^= ((uint16_t)data[i] << 8);
		for (int j = 0; j < 8; ++j) {
			crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
		}
	}
	return crc;
}
