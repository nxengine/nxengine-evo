<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="xml"/>
	<xsl:param name="DATE"/>

	<xsl:template match="/component/releases">
		<xsl:copy>
			<release>
				<xsl:attribute name="version">
					<xsl:value-of select="$VERSION"/>
				</xsl:attribute>

				<xsl:if test="boolean($DATE)">
					<xsl:attribute name="date">
						<xsl:value-of select="$DATE"/>
					</xsl:attribute>
				</xsl:if>
			</release>
			<xsl:apply-templates select="@* | node()" />
		</xsl:copy>
	</xsl:template>

	<xsl:template match="@* | node()">
		<xsl:copy>
			<xsl:apply-templates select="@* | node()" />
		</xsl:copy>
	</xsl:template>
</xsl:stylesheet>

