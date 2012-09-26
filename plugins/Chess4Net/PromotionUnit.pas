unit PromotionUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ExtCtrls,
  // Chess4net
  ChessRulesEngine, ChessBoardHeaderUnit, BitmapResUnit;

type
  TPromotionForm = class(TForm)
    PromFigImage: TImage;
    procedure FormShow(Sender: TObject);
    procedure PromFigImageMouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure FormKeyPress(Sender: TObject; var Key: Char);
  private
    m_iSquareSize: integer;
    m_BitmapRes: TBitmapRes;
    m_bmFigure: array[TFigure] of TBitmap;
    m_fig_color: TFigureColor;
    m_fig: TFigureName;
    procedure FLoadFigures;
  public
    function ShowPromotion(color: TFigureColor): TFigureName;
    constructor Create(AOwner: TComponent; BitmapRes: TBitmapRes); reintroduce;
    destructor Destroy; override;
  end;

implementation

{$R *.dfm}

const
  INDENT_SIZE = 2;

////////////////////////////////////////////////////////////////////////////////
// TPromotionForm 

procedure TPromotionForm.FormShow(Sender: TObject);
var
  k: byte;
begin
  if (m_iSquareSize <> m_BitmapRes.SquareSize) then
    FLoadFigures;

  // ���������� ���� � �������� �������
  Left := Mouse.CursorPos.X - m_iSquareSize div 2;
  Top := Mouse.CursorPos.Y - m_iSquareSize div 2;
  if (Left + Width > Screen.Width) then
    Left := Screen.Width - Width;

  with PromFigImage.Canvas do
    begin
      Brush.Color:= Color;
      FillRect(Rect(0,0, Width, PromFigImage.Height));

      Brush.Color:= clWhite;
      for k := 0 to 3 do
        FillRect(Rect((m_iSquareSize + INDENT_SIZE) * k, 0,
          (m_iSquareSize + INDENT_SIZE) * k + m_iSquareSize - 1, m_iSquareSize - 1));

      case m_fig_color of
        fcWhite:
          begin
            Draw(0, 0, m_bmFigure[WQ]);
            Draw(m_iSquareSize + 2, 0, m_bmFigure[WR]);
            Draw(2 * (m_iSquareSize + INDENT_SIZE), 0, m_bmFigure[WB]);
            Draw(3 * (m_iSquareSize + INDENT_SIZE), 0, m_bmFigure[WN]);
          end;
        fcBlack:
          begin
            Draw(0, 0, m_bmFigure[BQ]);
            Draw(m_iSquareSize + INDENT_SIZE, 0, m_bmFigure[BR]);
            Draw(2 * (m_iSquareSize + INDENT_SIZE), 0, m_bmFigure[BB]);
            Draw(3 * (m_iSquareSize + INDENT_SIZE), 0, m_bmFigure[BN]);
          end;
      end;
    end;
end;


function TPromotionForm.ShowPromotion(color: TFigureColor): TFigureName;
begin
  m_fig_color := color;
  ShowModal;
  Result := m_fig;
end;


procedure TPromotionForm.PromFigImageMouseUp(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  case X div (m_BitmapRes.SquareSize + 1) of
    1: m_fig:= R;
    2: m_fig:= B;
    3: m_fig:= N;
    else
      m_fig:= Q;
  end;
  Close;
end;

procedure TPromotionForm.FormKeyPress(Sender: TObject; var Key: Char);
begin
  Key:= UpCase(Key);
  case Key of
    'Q', '1', ' ', #13:
      m_fig:= Q;
    'R', '2':
      m_fig:= R;
    'B', '3':
      m_fig:= B;
    'N', '4':
      m_fig:= N;
    else
      exit;
  end;
  Close;
end;


constructor TPromotionForm.Create(AOwner: TComponent; BitmapRes: TBitmapRes);
begin
  self.FormStyle := (AOwner as TForm).FormStyle;
  inherited Create(AOwner);
  m_BitmapRes := BitmapRes;
  FLoadFigures;
end;


destructor TPromotionForm.Destroy;
var
  fig: TFigure;
begin
  for fig := Low(m_bmFigure) to High(m_bmFigure) do
    m_bmFigure[fig].Free;
  inherited;
end;


procedure TPromotionForm.FLoadFigures;
var
  fig: TFigure;
begin
  for fig := Low(m_bmFigure) to High(m_bmFigure) do
  begin
    FreeAndNil(m_bmFigure[fig]);
    m_BitmapRes.CreateFigureBitmap(fig, m_bmFigure[fig]);
  end;
  m_iSquareSize := m_BitmapRes.SquareSize;

  // Adjust size of the form
  PromFigImage.Width := 4 * m_iSquareSize + 3 * INDENT_SIZE;
  PromFigImage.Height := m_iSquareSize;
end;

end.
