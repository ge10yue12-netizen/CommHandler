#pragma once

#include <QByteArray>
#include <QVector>
// 联恒光科串口通信协议封装类
/**
 * @brief 联恒光科通信协议 v1.1 协议打包类
 * 封装了协议帧构造函数，用于客户端/服务端构造完整串口数据帧
 * 支持的帧类型包括：
 *  - 协议协商帧（0x08）
 *  - 控制命令帧（0x02）
 *  - 数据通道控制帧（0x07）
 *  - 模块数据帧（0x01）
 *  - 状态反馈帧（0x03）
 *  - 错误帧（0x04）
 *  - 数据轮询请求帧（0x05）
 *  - 外部数据推送频率帧（0x09）
 *
 * 注意：
 * - v1.1协议使用小端序（Little Endian），与v1.0的大端序不同
 * - 轮询请求帧（0x05）的响应使用模块数据帧（0x01），不存在独立的轮询响应帧
 */

class SerLhgkProtocol
{
public:
	/**
	* @param version_major 主版本号，如 1
	* @param version_minor 子版本号，如 1
	*/
	SerLhgkProtocol(uint8_t version_major = 1, uint8_t version_minor = 1);

	/**
	 * 构建协议协商帧（类型 0x08）
	 * @param isRequest true 表示请求帧，false 表示响应帧
	 */
	std::vector<uint8_t> buildProtocolNegotiationFrame(bool isRequest);

	/**
	 * 构建控制命令帧（类型 0x02）
	 * @param cmd 命令码（0x01 = 开始计算，0x02 = 停止计算）
	 */
	std::vector<uint8_t> buildControlCommandFrame(uint8_t cmd);

	/**
	 * 构建数据通道控制帧（类型 0x07）
	 * @param ctrl 控制码（0x01 = 开启数据流，0x02 = 关闭数据流）
	 */
	std::vector<uint8_t> buildDataChannelControlFrame(uint8_t ctrl);

	/**
	 * 构建模块数据帧（类型 0x01）
	 * @param moduleId 模块编号（如 0x01=标距, 0x06=外部数据）
	 * @param dataTypeId 数据类型编号（如 0x0B=力值）
	 * @param floatValues float 数组数据（float32 格式）
	 */
	std::vector<uint8_t> buildModuleDataFrame(const  std::vector < uint8_t>& moduleId, const  std::vector < uint8_t>& dataTypeId, const  std::vector<double>& data);
	/**
	 * 构建轮询请求帧（类型 0x05）
	 * 负载为空，引伸计收到后会返回模块数据帧（0x01）
	 */
	std::vector<uint8_t> buildPollingRequestFrame();

	/**
	 * 构建模块数据帧（类型 0x01）用于发送力值和温度（模块编号 0x06）
	 * @param force double 格式的力值（数据类型 0x0C）
	 * @param temp  double 格式的温度（数据类型 0x0D）
	 */
	std::vector<uint8_t> buildExternalDataFrame(double force, double temp);

	/**
	 * 构建状态反馈帧（类型 0x03）
	 * @param statusCode 状态码（如 0x00=空闲，0x01=计算中）
	 * @param message UTF-8 格式的状态描述文本
	 */
	std::vector<uint8_t> buildStatusResponseFrame(uint8_t statusCode, const std::string& message);

	/**
	 * 构建错误帧（类型 0x04）
	 * @param errCode 错误码（如 0x01=非法命令）
	 * @param errorMsg 错误描述
	 */
	std::vector<uint8_t> buildErrorFrame(uint8_t errCode, const std::string& errorMsg);

	/**
	 * 构建外部数据推送频率设置帧（类型 0x09）
	 * @param fps 推送频率（单位 fps）
	 */
	std::vector<uint8_t> buildExternalDataRateFrame(uint16_t fps);

	/**
	 * 获取帧类型（用于解包时识别），若不合法返回 0xFF
	 */
	uint8_t getFrameType(const std::vector<uint8_t>& frame);

	void getExternalData(const uint8_t& segmentCount, const std::vector<uint8_t>& payload, double& dForce, double& dTemp);

	bool checkAndParseFrame(const std::vector<uint8_t>& buf, uint8_t& type, std::vector<uint8_t>& payload);

	// 流式拼帧：buf 须已对齐 AA 55；返回完整帧总长；0=断帧待续；-1=当前同步点非法
	int probeCompleteFrameSize(const std::vector<uint8_t>& buf) const;

private:
	// 内部函数：构建完整帧格式
	std::vector<uint8_t> buildFrame(uint8_t type, const std::vector<uint8_t>& payload);
	std::vector<uint8_t> buildExternalFrame(uint8_t type, const std::vector<uint8_t>& payload);

	// 附加字段编码函数（v1.1使用小端序）
	void appendLittleEndian(std::vector<uint8_t>& buf, uint16_t val);
	void appendDoubleLE(std::vector<uint8_t>& buf, double d);

	// CRC16/XMODEM 校验算法
	uint16_t crc16_xmodem(const uint8_t* data, size_t length);

	uint8_t version_major_;
	uint8_t version_minor_;
};
