<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="xml" indent="yes" encoding="UTF-8"/>

<xsl:template name="Newline">
<xsl:text>
        </xsl:text>
</xsl:template>

<xsl:template match="@*|node()">
    <xsl:copy>
        <xsl:apply-templates select="@*|node()"/>
    </xsl:copy> 
</xsl:template>

<xsl:template match="//return_up_band/spot/carriers_distribution">
    <carriers_distribution>
    <xsl:call-template name="Newline" />
        <up_carriers access_type="ALOHA" category="Standard" ratio="10" symbol_rate="7.4E6" fmt_group="2"/>
    <xsl:call-template name="Newline" />
    </carriers_distribution>
</xsl:template>


</xsl:stylesheet>

